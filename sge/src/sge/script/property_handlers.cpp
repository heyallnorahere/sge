/*
   Copyright 2022 Nora Beda and SGE contributors

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "sgepch.h"
#include "sge/script/script_engine.h"
#include "sge/script/script_helpers.h"
#include "sge/scene/components.h"
#include "sge/scene/scene_serializer.h"
#include "sge/asset/project.h"

namespace sge {
    using edit_callback_t = void (*)(void*, void*, const std::string&);
    using serialize_callback_t = void (*)(void*, json&);
    using deserialize_callback_t = void (*)(void*, void*, const json&);

    struct handler_callbacks_t {
        edit_callback_t edit;
        serialize_callback_t serialize;
        deserialize_callback_t deserialize;
    };

    static struct {
        ref<scene> editor_scene;
        std::unordered_map<void*, handler_callbacks_t> callbacks;

        std::unordered_map<void*, std::vector<std::string>> enum_data;
        std::unordered_map<void*, std::vector<const char*>> enum_dropdown_data;
    } s_handler_data;

    namespace handlers {
        static ImGuiInputTextFlags get_input_text_flags(void* property) {
            ImGuiInputTextFlags flags = ImGuiInputTextFlags_None;

            if (script_helpers::is_property_read_only(property)) {
                flags |= ImGuiInputTextFlags_ReadOnly;
            }

            return flags;
        }

        static void edit_asset(void* instance, void* property, const std::string& label,
                               const std::string& asset_type,
                               const std::string& drag_drop_id = std::string()) {
            void* value = script_engine::get_property_value(instance, property);
            auto _asset = script_helpers::get_asset_from_object(value);

            std::string control_value;
            if (_asset) {
                fs::path asset_path = _asset->get_path();
                if (asset_path.empty()) {
                    control_value = "<no path>";
                } else {
                    control_value = asset_path.filename().string();
                }
            } else {
                control_value = "No " + asset_type + " set";
            }

            ImGui::InputText(label.c_str(), &control_value, ImGuiInputTextFlags_ReadOnly);
            if (!script_helpers::is_property_read_only(property) && ImGui::BeginDragDropTarget()) {
                std::string id = drag_drop_id;
                if (id.empty()) {
                    id = asset_type;
                }

                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(id.c_str())) {
                    fs::path path = std::string((const char*)payload->Data,
                                                payload->DataSize / sizeof(char) - 1);

                    auto& manager = project::get().get_asset_manager();
                    _asset = manager.get_asset(path);

                    if (!_asset) {
                        spdlog::error("failed to retrieve asset: {0}", path.string());
                    }

                    value = script_helpers::create_asset_object(_asset);
                    script_engine::set_property_value(instance, property, value);
                }

                ImGui::EndDragDropTarget();
            }
        }

        static void serialize_asset(void* object, json& data) {
            data = nullptr;

            if (object != nullptr) {
                auto _asset = (asset*)object;
                const auto& path = _asset->get_path();

                if (!path.empty()) {
                    if (path.is_absolute()) {
                        fs::path asset_dir = project::get().get_asset_dir();
                        data = asset_dir / path;
                    } else {
                        data = path;
                    }
                }
            }
        }

        static void deserialize_asset(void* instance, void* property, const json& data) {
            void* value = nullptr;
            if (!data.is_null()) {
                fs::path asset_path = data.get<fs::path>();
                auto& manager = project::get().get_asset_manager();

                ref<asset> _asset = manager.get_asset(asset_path);
                if (_asset) {
                    value = script_helpers::create_asset_object(_asset);
                }
            }

            script_engine::set_property_value(instance, property, value);
        }

        static const std::vector<const char*>& get_enum_dropdown_options(void* _class) {
            if (s_handler_data.enum_data.find(_class) == s_handler_data.enum_data.end()) {
                script_helpers::get_enum_value_names(_class, s_handler_data.enum_data[_class]);
                s_handler_data.enum_dropdown_data.clear();
            }

            if (s_handler_data.enum_dropdown_data.find(_class) ==
                s_handler_data.enum_dropdown_data.end()) {
                const auto& data = s_handler_data.enum_data.at(_class);

                auto& result = s_handler_data.enum_dropdown_data[_class];
                for (const auto& name : data) {
                    result.push_back(name.c_str());
                }
            }

            return s_handler_data.enum_dropdown_data.at(_class);
        }

        static void edit_enum(void* instance, void* property, const std::string& label) {
            void* value = script_engine::get_property_value(instance, property);
            int32_t index = script_engine::unbox_object<int32_t>(value);

            void* enum_type = script_engine::get_property_type(property);
            const auto& names = get_enum_dropdown_options(enum_type);

            if (script_helpers::is_property_read_only(property)) {
                const char* name = names[(size_t)index];
                ImGui::InputText(label.c_str(), (char*)name, ImGuiInputTextFlags_ReadOnly);
            } else {
                // being paranoid
                int combo_index = (int)index;

                if (ImGui::Combo(label.c_str(), &combo_index, names.data(), (int)names.size())) {
                    index = (int32_t)combo_index;
                    script_engine::set_property_value(instance, property, &index);
                }
            }
        }

        static void serialize_enum(void* object, void* _class, json& data) {
            void* enum_class = script_helpers::get_core_type("System.Enum");
            void* method = script_engine::get_method(enum_class, "GetName");

            void* reflection_type = script_engine::to_reflection_type(_class);
            void* returned = script_engine::call_method(nullptr, method, reflection_type, object);

            data = script_engine::from_managed_string(returned);
        }

        static void deserialize_enum(void* instance, void* property, const json& data) {
            std::string value = data.get<std::string>();

            void* _class = script_engine::get_property_type(property);
            int32_t enum_value = script_helpers::parse_enum(value, _class);

            if (enum_value < 0) {
                spdlog::error("invalid enum value: {0}", value);
            } else {
                script_engine::set_property_value(instance, property, &enum_value);
            }
        }

        namespace int_ {
            static void edit(void* instance, void* property, const std::string& label) {
                void* returned = script_engine::get_property_value(instance, property);
                int32_t value = script_engine::unbox_object<int32_t>(returned);

                if (ImGui::InputInt(label.c_str(), &value, get_input_text_flags(property))) {
                    script_engine::set_property_value(instance, property, &value);
                }
            }

            static void serialize(void* object, json& data) {
                data = script_engine::unbox_object<int32_t>(object);
            }

            static void deserialize(void* instance, void* property, const json& data) {
                int32_t value = data.get<int32_t>();
                script_engine::set_property_value(instance, property, &value);
            }
        } // namespace int_

        namespace float_ {
            static void edit(void* instance, void* property, const std::string& label) {
                void* returned = script_engine::get_property_value(instance, property);
                float value = script_engine::unbox_object<float>(returned);

                if (ImGui::InputFloat(label.c_str(), &value, get_input_text_flags(property))) {
                    script_engine::set_property_value(instance, property, &value);
                }
            }

            static void serialize(void* object, json& data) {
                data = script_engine::unbox_object<float>(object);
            }

            static void deserialize(void* instance, void* property, const json& data) {
                float value = data.get<float>();
                script_engine::set_property_value(instance, property, &value);
            }
        } // namespace float_

        namespace bool_ {
            static void edit(void* instance, void* property, const std::string& label) {
                void* returned = script_engine::get_property_value(instance, property);
                bool value = script_engine::unbox_object<bool>(returned);

                if (ImGui::Checkbox(label.c_str(), &value) &&
                    !script_helpers::is_property_read_only(property)) {
                    script_engine::set_property_value(instance, property, &value);
                }
            }

            static void serialize(void* object, json& data) {
                data = script_engine::unbox_object<bool>(object);
            }

            static void deserialize(void* instance, void* property, const json& data) {
                bool value = data.get<bool>();
                script_engine::set_property_value(instance, property, &value);
            }
        } // namespace bool_

        namespace string_ {
            static void edit(void* instance, void* property, const std::string& label) {
                void* managed_string = script_engine::get_property_value(instance, property);
                std::string string = script_engine::from_managed_string(managed_string);

                if (ImGui::InputText(label.c_str(), &string, get_input_text_flags(property))) {
                    managed_string = script_engine::to_managed_string(string);
                    script_engine::set_property_value(instance, property, managed_string);
                }
            }

            static void serialize(void* object, json& data) {
                data = script_engine::from_managed_string(object);
            }

            static void deserialize(void* instance, void* property, const json& data) {
                std::string value = data.get<std::string>();
                void* managed_string = script_engine::to_managed_string(value);
                script_engine::set_property_value(instance, property, managed_string);
            }
        } // namespace string_

        namespace entity_ {
            static void edit(void* instance, void* property, const std::string& label) {
                void* entity_object = script_engine::get_property_value(instance, property);
                std::string tag;

                if (entity_object != nullptr) {
                    entity e = script_helpers::get_entity_from_object(entity_object);

                    if (e.has_all<tag_component>()) {
                        tag = e.get_component<tag_component>().tag;
                    } else {
                        tag = "<no tag>";
                    }
                } else {
                    tag = "No entity set";
                }

                ImGui::InputText(label.c_str(), &tag, ImGuiInputTextFlags_ReadOnly);
                if (!script_helpers::is_property_read_only(property) &&
                    ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("entity")) {
                        guid id = *(guid*)payload->Data;

                        auto _scene = s_handler_data.editor_scene;
                        if (!_scene) {
                            throw std::runtime_error("no scene is set!");
                        }

                        entity found_entity = _scene->find_guid(id);
                        if (!found_entity) {
                            throw std::runtime_error("an invalid guid was given!");
                        }

                        void* entity_object = script_helpers::create_entity_object(found_entity);
                        script_engine::set_property_value(instance, property, entity_object);
                    }

                    ImGui::EndDragDropTarget();
                }
            }

            static void serialize(void* object, json& data) {
                if (object != nullptr) {
                    entity e = script_helpers::get_entity_from_object(object);
                    data = e.get_guid();
                } else {
                    data = nullptr;
                }
            }

            static void deserialize(void* instance, void* property, const json& data) {
                if (!data.is_null()) {
                    guid id = data.get<guid>();

                    auto current_serialization = serialization_data::current();
                    if (current_serialization == nullptr) {
                        throw std::runtime_error("no serialization is active!");
                    }

                    current_serialization->post_deserialize.push([instance, property, id,
                                                                  current_serialization]() mutable {
                        entity found_entity = current_serialization->_scene->find_guid(id);
                        if (!found_entity) {
                            throw std::runtime_error("a nonexistent entity was specified");
                        }

                        void* entity_object = script_helpers::create_entity_object(found_entity);
                        script_engine::set_property_value(instance, property, entity_object);
                    });
                } else {
                    script_engine::set_property_value(instance, property, (void*)nullptr);
                }
            }
        } // namespace entity_

        namespace texture_2d_ {
            static void edit(void* instance, void* property, const std::string& label) {
                edit_asset(instance, property, label, "texture", "texture_2d");
            }

            static void serialize(void* object, json& data) { serialize_asset(object, data); }

            static void deserialize(void* instance, void* property, const json& data) {
                deserialize_asset(instance, property, data);
            }
        }; // namespace texture_2d_

        namespace prefab_ {
            static void edit(void* instance, void* property, const std::string& label) {
                edit_asset(instance, property, label, "prefab");
            }

            static void serialize(void* object, json& data) { serialize_asset(object, data); }

            static void deserialize(void* instance, void* property, const json& data) {
                deserialize_asset(instance, property, data);
            }
        }; // namespace prefab_
    }      // namespace handlers

    void script_helpers::set_editor_scene(ref<scene> _scene) {
        s_handler_data.editor_scene = _scene;
    }

    void script_helpers::show_property_control(void* instance, void* property,
                                               const std::string& label) {
        if (!is_property_serializable(property)) {
            return;
        }

        void* _class = script_engine::get_property_type(property);
        if (type_is_enum(_class)) {
            handlers::edit_enum(instance, property, label);
            return;
        }

        if (s_handler_data.callbacks.find(_class) == s_handler_data.callbacks.end()) {
            // type isn't supported yet
            return;
        }

        s_handler_data.callbacks.at(_class).edit(instance, property, label);
    }

    void script_helpers::serialize_property(void* instance, void* property, json& data) {
        if (!is_property_serializable(property) || is_property_read_only(property)) {
            return;
        }

        void* _class = script_engine::get_property_type(property);
        void* object = script_engine::get_property_value(instance, property);
        if (type_is_enum(_class)) {
            handlers::serialize_enum(object, _class, data);
            return;
        }

        if (s_handler_data.callbacks.find(_class) == s_handler_data.callbacks.end()) {
            return;
        }

        s_handler_data.callbacks.at(_class).serialize(object, data);
    }

    void script_helpers::deserialize_property(void* instance, void* property, const json& data) {
        if (!is_property_serializable(property) || is_property_read_only(property)) {
            return;
        }

        void* _class = script_engine::get_property_type(property);
        if (type_is_enum(_class)) {
            handlers::deserialize_enum(instance, property, data);
        }

        if (s_handler_data.callbacks.find(_class) == s_handler_data.callbacks.end()) {
            return;
        }

        s_handler_data.callbacks.at(_class).deserialize(instance, property, data);
    }

    static void register_property_handler(const std::string& managed,
                                          const handler_callbacks_t& handler) {
        bool is_scriptcore = managed.find("SGE.") != std::string::npos;
        void* _class = script_helpers::get_core_type(managed, is_scriptcore);

        if (_class != nullptr) {
            s_handler_data.callbacks.insert(std::make_pair(_class, handler));
        }
    }

#define HANDLER_FUNC(type, name) handlers::type##_::name
#define REGISTER_HANDLER(native, managed)                                                          \
    {                                                                                              \
        handler_callbacks_t handler;                                                               \
        handler.edit = HANDLER_FUNC(native, edit);                                                 \
        handler.serialize = HANDLER_FUNC(native, serialize);                                       \
        handler.deserialize = HANDLER_FUNC(native, deserialize);                                   \
                                                                                                   \
        register_property_handler(managed, handler);                                               \
    }

    void script_helpers::register_property_handlers() {
        s_handler_data.callbacks.clear();
        s_handler_data.enum_data.clear();
        s_handler_data.enum_dropdown_data.clear();

        REGISTER_HANDLER(int, "System.Int32");
        REGISTER_HANDLER(float, "System.Single");
        REGISTER_HANDLER(bool, "System.Boolean");
        REGISTER_HANDLER(string, "System.String");
        REGISTER_HANDLER(entity, "SGE.Entity");
        REGISTER_HANDLER(texture_2d, "SGE.Texture2D");
        REGISTER_HANDLER(prefab, "SGE.Prefab");
    }

#undef REGISTER_HANDLER
#undef HANDLER_FUNC
} // namespace sge