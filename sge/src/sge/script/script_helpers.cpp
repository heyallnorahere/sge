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
    void script_helpers::init(void* helpers_class) { managed_helpers_class = helpers_class; }

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

    bool script_helpers::is_property_serializable(void* property) {
        bool serializable = true;

        void* scriptcore = script_engine::get_assembly(0);
        void* unserialized_attribute = script_engine::get_class(scriptcore, "SGE.UnserializedAttribute");
        serializable &= !property_has_attribute(property, unserialized_attribute);

        uint32_t accessors = script_engine::get_property_accessors(property);
        serializable &= ((accessors & (property_accessor_get | property_accessor_set)) !=
                         property_accessor_none);

        uint32_t visibility = script_engine::get_property_visibility(property);
        serializable &=
            ((visibility & member_visibility_flags_public) != member_visibility_flags_none);

        return serializable;
    }

    void* script_helpers::create_entity_object(entity e) {
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
        void* scene_ptr = script_engine::unbox_object<void*>(value);

        return entity((entt::entity)entity_id, (scene*)scene_ptr);
    }

    void* script_helpers::get_core_type(const std::string& name, bool scriptcore) {
        void* assembly =
            scriptcore ? script_engine::get_assembly(0) : script_engine::get_mscorlib();
        return script_engine::get_class(assembly, name);
    }

    void* script_helpers::create_event_object(event& e) {
        void* method = script_engine::get_method(managed_helpers_class, "CreateEvent");

        event_id id = e.get_id();
        void* ptr = &e;

        return script_engine::call_method(nullptr, method, &ptr, &id);
    }
} // namespace sge