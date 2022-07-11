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
#include "sge/script/script_helpers.h"
#include "sge/script/script_engine.h"
#include "sge/script/garbage_collector.h"
namespace sge {
    static void* managed_helpers_class = nullptr;
    void script_helpers::init() {
        managed_helpers_class = get_core_type("SGE.Helpers", true);
        register_property_handlers();
    }

    void* script_helpers::get_class() { return managed_helpers_class; }

    void script_helpers::report_exception(void* exception) {
        void* method = script_engine::get_method(managed_helpers_class, "ReportException");
        script_engine::call_method(nullptr, method, exception);
    }

    bool script_helpers::property_has_attribute(void* property, void* attribute_type) {
        void* reflection_property = script_engine::to_reflection_property(property);
        void* reflection_type = script_engine::to_reflection_type(attribute_type);

        void* method = script_engine::get_method(managed_helpers_class, "PropertyHasAttribute");
        void* returned =
            script_engine::call_method(nullptr, method, reflection_property, reflection_type);

        return script_engine::unbox_object<bool>(returned);
    }

    uint32_t script_helpers::get_property_attribute(void* property, void* attribute_type) {
        void* reflection_property = script_engine::to_reflection_property(property);
        void* reflection_type = script_engine::to_reflection_type(attribute_type);

        void* method = script_engine::get_method(managed_helpers_class, "GetPropertyAttribute");
        void* returned =
            script_engine::call_method(nullptr, method, reflection_property, reflection_type);

        uint32_t handle = 0;
        if (returned != nullptr) {
            handle = garbage_collector::create_ref(returned);
        }

        return handle;
    }

    void script_helpers::get_enum_value_names(void* _class, std::vector<std::string>& names) {
        void* method = script_engine::get_method(managed_helpers_class, "GetEnumValueNames");

        void* reflection_type = script_engine::to_reflection_type(_class);
        void* list = script_engine::call_method(nullptr, method, reflection_type);

        names.clear();
        if (list != nullptr) {
            void* list_type = script_engine::get_class_from_object(list);

            void* count_property = script_engine::get_property(list_type, "Count");
            void* result = script_engine::get_property_value(list, count_property);
            int32_t count = script_engine::unbox_object<int32_t>(result);

            void* item_property = script_engine::get_property(list_type, "Item");
            for (int32_t i = 0; i < count; i++) {
                result = script_engine::get_property_value(list, item_property, &i);
                names.push_back(script_engine::from_managed_string(result));
            }
        }
    }

    int32_t script_helpers::parse_enum(const std::string& value, void* enum_type) {
        void* reflection_type = script_engine::to_reflection_type(enum_type);
        void* managed_string = script_engine::to_managed_string(value);
        static bool ignore_case = true;

        void* method = script_engine::get_method(managed_helpers_class, "ParseEnum");
        void* returned = script_engine::call_method(nullptr, method, reflection_type,
                                                    managed_string, &ignore_case);

        if (returned != nullptr) {
            return script_engine::unbox_object<int32_t>(returned);
        } else {
            return -1;
        }
    }

    bool script_helpers::is_property_serializable(void* property) {
        bool serializable = true;

        void* scriptcore = script_engine::get_assembly(0);
        void* unserialized_attribute =
            script_engine::get_class(scriptcore, "SGE.UnserializedAttribute");
        serializable &= !property_has_attribute(property, unserialized_attribute);

        uint32_t accessors = script_engine::get_property_accessors(property);
        serializable &= ((accessors & property_accessor_get) != property_accessor_none);

        uint32_t visibility = script_engine::get_property_visibility(property);
        serializable &=
            ((visibility & member_visibility_flags_public) != member_visibility_flags_none);

        return serializable;
    }

    bool script_helpers::is_property_read_only(void* property) {
        uint32_t accessors = script_engine::get_property_accessors(property);
        return (accessors & property_accessor_set) == property_accessor_none;
    }

    void* script_helpers::create_entity_object(entity e) {
        if (!e) {
            return nullptr;
        }

        void* scriptcore = script_engine::get_assembly(0);
        void* scene_class = script_engine::get_class(scriptcore, "SGE.Scene");
        if (scene_class == nullptr) {
            throw std::runtime_error("could not find SGE.Scene!");
        }

        void* scene_constructor = script_engine::get_method(scene_class, ".ctor");
        if (scene_constructor == nullptr) {
            throw std::runtime_error("could not find the Scene object constructor!");
        }

        scene* _scene = e.get_scene();

        void* scene_instance = script_engine::alloc_object(scene_class);
        script_engine::call_method(scene_instance, scene_constructor, &_scene);

        void* entity_class = script_engine::get_class(scriptcore, "SGE.Entity");
        if (entity_class == nullptr) {
            throw std::runtime_error("could not find SGE.Entity!");
        }

        void* entity_constructor = script_engine::get_method(entity_class, ".ctor");
        if (entity_constructor == nullptr) {
            throw std::runtime_error("could not find the Entity object constructor!");
        }

        uint32_t id = (uint32_t)e;

        void* entity_instance = script_engine::alloc_object(entity_class);
        script_engine::call_method(entity_instance, entity_constructor, &id, scene_instance);
        return entity_instance;
    }

    entity script_helpers::get_entity_from_object(void* object) {
        if (object == nullptr) {
            return entity();
        }

        void* scriptcore = script_engine::get_assembly(0);
        void* entity_class = script_engine::get_class(scriptcore, "SGE.Entity");
        if (entity_class == nullptr) {
            throw std::runtime_error("could not find SGE.Entity!");
        }

        void* field = script_engine::get_field(entity_class, "mID");
        void* value = script_engine::get_field_value(object, field);
        uint32_t entity_id = script_engine::unbox_object<uint32_t>(value);

        field = script_engine::get_field(entity_class, "mScene");
        void* scene_object = script_engine::get_field_value(object, field);

        void* scene_class = script_engine::get_class(scriptcore, "SGE.Scene");
        if (scene_class == nullptr) {
            throw std::runtime_error("could not find SGE.Scene!");
        }

        field = script_engine::get_field(scene_class, "mNativeAddress");
        value = script_engine::get_field_value(scene_object, field);
        scene* _scene = script_engine::unbox_object<scene*>(value);

        return entity((entt::entity)entity_id, _scene);
    }

    void* script_helpers::create_asset_object(ref<asset> _asset) {
        if (!_asset) {
            return nullptr;
        }

        void* asset_class = get_core_type("SGE.Asset", true);
        void* method = script_engine::get_method(asset_class, "FromPointer");

        asset* ptr = _asset.raw();
        return script_engine::call_method(nullptr, method, &ptr);
    }

    ref<asset> script_helpers::get_asset_from_object(void* object) {
        if (object == nullptr) {
            return nullptr;
        }

        void* asset_class = get_core_type("SGE.Asset", true);
        void* field = script_engine::get_field(asset_class, "mAddress");

        void* value = script_engine::get_field_value(object, field);
        return script_engine::unbox_object<asset*>(value);
    }

    void* script_helpers::get_core_type(const std::string& name, bool scriptcore) {
        void* assembly =
            scriptcore ? script_engine::get_assembly(0) : script_engine::get_mscorlib();
        return script_engine::get_class(assembly, name);
    }

    std::string script_helpers::get_type_name_safe(void* _class) {
        void* method = script_engine::get_method(managed_helpers_class, "GetTypeNameSafe");
        void* reflection_type = script_engine::to_reflection_type(_class);

        void* returned = script_engine::call_method(nullptr, method, reflection_type);
        return script_engine::from_managed_string(returned);
    }

    size_t script_helpers::get_type_size(void* _class) {
        void* method = script_engine::get_method(managed_helpers_class, "GetTypeSize");
        void* reflection_type = script_engine::to_reflection_type(_class);

        void* returned = script_engine::call_method(nullptr, method, reflection_type);
        return (size_t)script_engine::unbox_object<int32_t>(returned);
    }

    bool script_helpers::type_is_array(void* _class) {
        void* type_class = get_core_type("System.Type");
        void* property = script_engine::get_property(type_class, "IsArray");

        void* reflection_type = script_engine::to_reflection_type(_class);
        void* returned = script_engine::get_property_value(reflection_type, property);

        return script_engine::unbox_object<bool>(returned);
    }

    bool script_helpers::type_is_enum(void* _class) {
        void* type_class = get_core_type("System.Type");
        void* property = script_engine::get_property(type_class, "IsEnum");

        void* reflection_type = script_engine::to_reflection_type(_class);
        void* returned = script_engine::get_property_value(reflection_type, property);

        return script_engine::unbox_object<bool>(returned);
    }

    bool script_helpers::type_extends(void* derived, void* base) {
        void* reflection_derived = script_engine::to_reflection_type(derived);
        void* reflection_base = script_engine::to_reflection_type(base);

        void* method = script_engine::get_method(managed_helpers_class, "ExtendsImpl");
        void* result =
            script_engine::call_method(nullptr, method, reflection_derived, reflection_base);

        return script_engine::unbox_object<bool>(result);
    }

    void* script_helpers::create_event_object(event& e) {
        void* method = script_engine::get_method(managed_helpers_class, "CreateEvent");

        event_id id = e.get_id();
        void* ptr = &e;

        return script_engine::call_method(nullptr, method, &ptr, &id);
    }

    void* script_helpers::create_list_object(void* element_type) {
        void* method = script_engine::get_method(managed_helpers_class, "CreateListObject");
        void* reflection_type = script_engine::to_reflection_type(element_type);

        return script_engine::call_method(nullptr, method, reflection_type);
    }
} // namespace sge