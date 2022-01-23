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
#include "sge/scene/scene_serializer.h"
#include "sge/scene/entity.h"
#include "sge/scene/components.h"
#include "sge/script/script_engine.h"
#include "sge/script/script_helpers.h"
#include "sge/script/garbage_collector.h"
#include "sge/asset/json.h"
#include "sge/asset/project.h"
namespace sge {
    struct serialization_data {
        std::queue<std::function<void()>> post_deserialize;
        ref<scene> _scene;
        entity current_entity;
    };
    static std::unique_ptr<serialization_data> current_serialization;

    void to_json(json& data, const id_component& comp) { data = comp.id; }
    void from_json(const json& data, id_component& comp) { comp.id = data.get<guid>(); }

    void to_json(json& data, const tag_component& comp) { data = comp.tag; }
    void from_json(const json& data, tag_component& comp) { comp.tag = data.get<std::string>(); }

    void to_json(json& data, const transform_component& comp) {
        data["translation"] = comp.translation;
        data["rotation"] = comp.rotation;
        data["scale"] = comp.scale;
    }

    void from_json(const json& data, transform_component& comp) {
        comp.translation = data["translation"].get<glm::vec2>();
        comp.rotation = data["rotation"].get<float>();
        comp.scale = data["scale"].get<glm::vec2>();
    }

    void to_json(json& data, const camera_component& camera) {
        data["primary"] = camera.primary;

        switch (camera.camera.get_projection_type()) {
        case projection_type::orthographic:
            data["view_size"] = camera.camera.get_orthographic_size();
            data["vertical_fov"] = nullptr;
            data["near_clip"] = camera.camera.get_orthographic_near_plane();
            data["far_clip"] = camera.camera.get_orthographic_far_plane();
            break;
        case projection_type::perspective:
            data["view_size"] = nullptr;
            data["vertical_fov"] = camera.camera.get_vertical_fov();
            data["near_clip"] = camera.camera.get_perspective_near_plane();
            data["far_clip"] = camera.camera.get_perspective_far_plane();
            break;
        default:
            throw std::runtime_error("invalid projection type!");
        }
    }

    void from_json(const json& data, camera_component& camera) {
        camera.primary = data["primary"].get<bool>();

        float near_clip = data["near_clip"].get<float>();
        float far_clip = data["far_clip"].get<float>();

        if (!data["view_size"].is_null()) {
            float view_size = data["view_size"].get<float>();

            camera.camera.set_orthographic(view_size, near_clip, far_clip);
        } else if (!data["vertical_fov"].is_null()) {
            float fov = data["vertical_fov"].get<float>();

            camera.camera.set_perspective(fov, near_clip, far_clip);
        } else {
            throw std::runtime_error("invalid projection type!");
        }
    }

    void to_json(json& data, const sprite_renderer_component& comp) {
        data["color"] = comp.color;
        data["texture"] = nullptr;

        if (comp.texture) {
            if (!project::loaded()) {
                throw std::runtime_error("cannot serialize assets without a project loaded!");
            }

            fs::path path = comp.texture->get_path();
            if (!path.empty()) {
                if (path.is_absolute()) {
                    fs::path asset_dir = project::get().get_asset_dir();
                    path = path.lexically_relative(asset_dir);
                }

                data["texture"] = path;
            }
        }
    }

    void from_json(const json& data, sprite_renderer_component& comp) {
        comp.color = data["color"].get<glm::vec4>();

        if (!data["texture"].is_null()) {
            if (!project::loaded()) {
                throw std::runtime_error("cannot deserialize assets without a project loaded!");
            }

            fs::path path = data["texture"].get<fs::path>();
            auto _asset = project::get().get_asset_manager().get_asset(path);

            if (_asset) {
                comp.texture = _asset.as<texture_2d>();
            }
        }
    }

    void to_json(json& data, const rigid_body_component& rb) {
        data["fixed_rotation"] = rb.fixed_rotation;

        using body_type = rigid_body_component::body_type;
        switch (rb.type) {
        case body_type::static_:
            data["type"] = "static";
            break;
        case body_type::kinematic:
            data["type"] = "kinematic";
            break;
        case body_type::dynamic:
            data["type"] = "dynamic";
            break;
        default:
            throw std::runtime_error("invalid body type!");
        }
    }

    static const std::unordered_map<std::string, rigid_body_component::body_type> body_type_map = {
        { "static", rigid_body_component::body_type::static_ },
        { "kinematic", rigid_body_component::body_type::kinematic },
        { "dynamic", rigid_body_component::body_type::dynamic },
    };

    void from_json(const json& data, rigid_body_component& rb) {
        rb.fixed_rotation = data["fixed_rotation"].get<bool>();

        std::string body_type = data["type"].get<std::string>();
        if (body_type_map.find(body_type) == body_type_map.end()) {
            throw std::runtime_error("invalid body type!");
        }
        rb.type = body_type_map.at(body_type);
    }

    void to_json(json& data, const box_collider_component& bc) {
        data["density"] = bc.density;
        data["friction"] = bc.friction;
        data["restitution"] = bc.restitution;
        data["restitution_threashold"] = bc.restitution_threashold;
        data["size"] = bc.size;
    }

    void from_json(const json& data, box_collider_component& bc) {
        bc.density = data["density"].get<float>();
        bc.friction = data["friction"].get<float>();
        bc.restitution = data["restitution"].get<float>();
        bc.restitution_threashold = data["restitution_threashold"].get<float>();
        bc.size = data["size"].get<glm::vec2>();
    }

    static json serialize_int(void* object) { return script_engine::unbox_object<int32_t>(object); }
    static json serialize_float(void* object) { return script_engine::unbox_object<float>(object); }
    static json serialize_bool(void* object) { return script_engine::unbox_object<bool>(object); }

    static json serialize_string(void* object) {
        return script_engine::from_managed_string(object);
    }

    static json serialize_entity(void* object) {
        if (object != nullptr) {
            entity e = script_helpers::get_entity_from_object(object);
            return e.get_guid();
        } else {
            return nullptr;
        }
    }

    void to_json(json& data, const script_component& component) {
        static std::unordered_map<void*, json (*)(void*)> serialization_callbacks = {
            { script_helpers::get_core_type("System.Int32"), serialize_int },
            { script_helpers::get_core_type("System.Single"), serialize_float },
            { script_helpers::get_core_type("System.Boolean"), serialize_bool },
            { script_helpers::get_core_type("System.String"), serialize_string },
            { script_helpers::get_core_type("SGE.Entity", true), serialize_entity }
        };

        if (component._class == nullptr) {
            data = nullptr;
            return;
        }

        data["script_name"] = component.class_name;
        data["properties"] = nullptr;

        if (component.gc_handle != 0) {
            void* instance = garbage_collector::get_ref_data(component.gc_handle);

            std::vector<void*> properties;
            script_engine::iterate_properties(component._class, properties);

            if (!properties.empty()) {
                json property_data;
                for (void* property : properties) {
                    if (!script_helpers::is_property_serializable(property)) {
                        continue;
                    }

                    void* property_type = script_engine::get_property_type(property);
                    if (serialization_callbacks.find(property_type) ==
                        serialization_callbacks.end()) {
                        continue;
                    }

                    std::string property_name = script_engine::get_property_name(property);
                    void* property_value = script_engine::get_property_value(instance, property);

                    auto callback = serialization_callbacks[property_type];
                    property_data[property_name] = callback(property_value);
                }

                data["properties"] = property_data;
            }
        }
    }

    static void deserialize_int(void* instance, void* property, json data) {
        int32_t value = data.get<int32_t>();
        script_engine::set_property_value(instance, property, &value);
    }

    static void deserialize_float(void* instance, void* property, json data) {
        float value = data.get<float>();
        script_engine::set_property_value(instance, property, &value);
    }

    static void deserialize_bool(void* instance, void* property, json data) {
        bool value = data.get<bool>();
        script_engine::set_property_value(instance, property, &value);
    }

    static void deserialize_string(void* instance, void* property, json data) {
        std::string value = data.get<std::string>();
        void* managed_string = script_engine::to_managed_string(value);
        script_engine::set_property_value(instance, property, managed_string);
    }

    static void deserialize_entity(void* instance, void* property, json data) {
        if (!data.is_null()) {
            guid id = data.get<guid>();

            current_serialization->post_deserialize.push([instance, property, id]() mutable {
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

    struct script_deserializer {
        void operator()(json property_data, script_component& component) {
            static std::unordered_map<void*, void (*)(void*, void*, json)>
                deserialization_callbacks = {
                    { script_helpers::get_core_type("System.Int32"), deserialize_int },
                    { script_helpers::get_core_type("System.Single"), deserialize_float },
                    { script_helpers::get_core_type("System.Boolean"), deserialize_bool },
                    { script_helpers::get_core_type("System.String"), deserialize_string },
                    { script_helpers::get_core_type("SGE.Entity", true), deserialize_entity },
                };

            entity current_entity = current_serialization->current_entity;
            component.verify_script(current_entity);
            void* instance = garbage_collector::get_ref_data(component.gc_handle);

            std::vector<void*> properties;
            script_engine::iterate_properties(component._class, properties);

            for (void* property : properties) {
                if (!script_helpers::is_property_serializable(property)) {
                    continue;
                }

                void* property_type = script_engine::get_property_type(property);
                if (deserialization_callbacks.find(property_type) ==
                    deserialization_callbacks.end()) {
                    continue;
                }

                std::string property_name = script_engine::get_property_name(property);
                if (property_data.find(property_name) == property_data.end()) {
                    continue;
                }

                auto callback = deserialization_callbacks[property_type];
                callback(instance, property, property_data[property_name]);
            }
        }
    };

    void from_json(const json& data, script_component& component) {
        component.class_name = data["script_name"].get<std::string>();

        // todo: make this more dynamic
        void* app_assembly = script_engine::get_assembly(1);
        component._class = script_engine::get_class(app_assembly, component.class_name);

        json property_data = data["properties"];
        if (!property_data.is_null()) {
            script_deserializer deserializer;
            deserializer(property_data, component);
        }
    }

    template <typename T>
    static void serialize_component(entity e, const std::string& key, json& data) {
        if (e.has_all<T>()) {
            data[key] = e.get_component<T>();
        } else {
            data[key] = nullptr;
        }
    }

    template <typename T>
    static void deserialize_component(entity e, const std::string& key, const json& data) {
        if (data.find(key) == data.end()) {
            return;
        }

        if (data[key].is_null()) {
            return;
        }

        T* component;
        if (e.has_all<T>()) {
            component = &e.get_component<T>();
        } else {
            component = &e.add_component<T>();
        }

        *component = data[key].get<T>();
    }

    scene_serializer::scene_serializer(ref<scene> _scene) { m_scene = _scene; }

    void scene_serializer::serialize(const fs::path& path) {
        if (current_serialization) {
            throw std::runtime_error("please do not use two serializers at the same time");
        }

        current_serialization = std::make_unique<serialization_data>();
        current_serialization->_scene = m_scene;

        json data;

        // todo: global scene data

        json entities;
        m_scene->for_each([&](entity current) {
            json entity_data;
            current_serialization->current_entity = current;

            serialize_component<id_component>(current, "guid", entity_data);
            serialize_component<tag_component>(current, "tag", entity_data);
            serialize_component<transform_component>(current, "transform", entity_data);
            serialize_component<camera_component>(current, "camera", entity_data);
            serialize_component<sprite_renderer_component>(current, "sprite", entity_data);
            serialize_component<rigid_body_component>(current, "rigid_body", entity_data);
            serialize_component<box_collider_component>(current, "box_collider", entity_data);
            serialize_component<script_component>(current, "script", entity_data);

            entities.push_back(entity_data);
        });
        data["entities"] = entities;

        current_serialization.reset();

        std::ofstream stream(path);
        stream << data.dump(4) << std::flush;
        stream.close();
    }

    void scene_serializer::deserialize(const fs::path& path) {
        if (!fs::exists(path)) {
            spdlog::warn("attempted to deserialize nonexistent scene: {0}", path.string());
            return;
        }

        if (current_serialization) {
            throw std::runtime_error("please do not use two serializers at the same time");
        }

        current_serialization = std::make_unique<serialization_data>();
        current_serialization->_scene = m_scene;

        json data;
        std::ifstream stream(path);
        stream >> data;
        stream.close();

        m_scene->clear();

        for (const auto& entity_data : data["entities"]) {
            entity e = m_scene->create_entity();
            current_serialization->current_entity = e;

            deserialize_component<id_component>(e, "guid", entity_data);
            deserialize_component<tag_component>(e, "tag", entity_data);
            deserialize_component<transform_component>(e, "transform", entity_data);
            deserialize_component<camera_component>(e, "camera", entity_data);
            deserialize_component<sprite_renderer_component>(e, "sprite", entity_data);
            deserialize_component<rigid_body_component>(e, "rigid_body", entity_data);
            deserialize_component<box_collider_component>(e, "box_collider", entity_data);
            deserialize_component<script_component>(e, "script", entity_data);
        }

        while (!current_serialization->post_deserialize.empty()) {
            auto task = current_serialization->post_deserialize.front();
            current_serialization->post_deserialize.pop();

            task();
        }

        current_serialization.reset();
    }
} // namespace sge
