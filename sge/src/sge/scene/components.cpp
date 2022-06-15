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
#include "sge/scene/components.h"
#include "sge/script/script_engine.h"
#include "sge/script/script_helpers.h"
#include "sge/script/garbage_collector.h"

using namespace entt::literals;

namespace sge {
    META_REGISTER {
        transform_component::meta_register();
        sprite_renderer_component::meta_register();
        camera_component::meta_register();
        native_script_component::meta_register();
        rigid_body_component::meta_register();
        box_collider_component::meta_register();
        script_component::meta_register();
    }

    script_component& script_component::clone(const entity& src, const entity& dst,
                                              void* csrc_void) {
        auto& csrc = *reinterpret_cast<script_component*>(csrc_void);
        scene* dst_scene = dst.get_scene();

        script_component cdst;
        cdst._class = csrc._class;
        cdst.class_name = csrc.class_name;
        cdst.enabled = csrc.enabled;

        if (csrc.gc_handle != 0) {
            cdst.verify_script(dst);

            void* src_instance = garbage_collector::get_ref_data(csrc.gc_handle);
            void* dst_instance = garbage_collector::get_ref_data(cdst.gc_handle);

            std::vector<void*> properties;
            script_engine::iterate_properties(csrc._class, properties);

            void* entity_class = script_helpers::get_core_type("SGE.Entity", true);
            void* asset_class = script_helpers::get_core_type("SGE.Asset", true);

            for (void* property : properties) {
                if (!script_helpers::is_property_serializable(property)) {
                    continue;
                }

                void* property_type = script_engine::get_property_type(property);
                void* value = script_engine::get_property_value(src_instance, property);

                if (script_engine::is_value_type(property_type)) {
                    // good enough for now
                    const void* data = script_engine::unbox_object(value);
                    script_engine::set_property_value(dst_instance, property, (void*)data);
                } else {
                    if (value != nullptr) {
                        if (property_type == entity_class) {
                            entity native_entity = script_helpers::get_entity_from_object(value);

                            entity found_entity = dst_scene->find_guid(native_entity.get_guid());
                            if (!found_entity) {
                                throw std::runtime_error(
                                    "script_component did not clone correctly!");
                            }

                            void* entity_object =
                                script_helpers::create_entity_object(found_entity);
                            script_engine::set_property_value(dst_instance, property,
                                                              entity_object);
                        } else if (property_type == asset_class) {
                            ref<asset> _asset = script_helpers::get_asset_from_object(value);
                            void* asset_object = script_helpers::create_asset_object(_asset);

                            script_engine::set_property_value(dst_instance, property, asset_object);
                        } else {
                            void* new_value = script_engine::clone_object(value);
                            script_engine::set_property_value(dst_instance, property, new_value);
                        }
                    } else {
                        // don't know how else to clone it
                        script_engine::set_property_value(dst_instance, property, (void*)nullptr);
                    }
                }
            }
        }

        return dst.add_component<script_component>(cdst);
    }

    void script_component::verify_script(entity e) {
        if (_class != nullptr && gc_handle == 0) {
            void* instance = script_engine::alloc_object(_class);

            if (script_engine::get_method(_class, ".ctor") != nullptr) {
                void* constructor = script_engine::get_method(_class, ".ctor()");
                if (constructor != nullptr) {
                    script_engine::call_method(instance, constructor);
                } else {
                    class_name_t name_data;
                    script_engine::get_class_name(_class, name_data);
                    std::string full_name = script_engine::get_string(name_data);

                    throw std::runtime_error("could not find a suitable constructor for script: " +
                                             full_name);
                }
            } else {
                script_engine::init_object(instance);
            }

            void* entity_instance = script_helpers::create_entity_object(e);
            void* entity_field = script_engine::get_field(_class, "__internal_mEntity");
            script_engine::set_field_value(instance, entity_field, entity_instance);

            gc_handle = garbage_collector::create_ref(instance);
        }
    }

    void script_component::remove_script() {
        if (_class != nullptr) {
            if (gc_handle != 0) {
                garbage_collector::destroy_ref(gc_handle);
                gc_handle = 0;
            }

            _class = nullptr;
        }
    }
} // namespace sge
