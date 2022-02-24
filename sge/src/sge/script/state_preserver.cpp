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
#include "sge/script/state_preserver.h"
#include "sge/script/script_engine.h"
#include "sge/script/garbage_collector.h"
namespace sge {
    state_preserver::state_preserver(const std::vector<void*>& reloaded_assemblies) {
        m_reloaded_assemblies = reloaded_assemblies;

        retrieve_data();
        destroy_objects();
    }

    state_preserver::~state_preserver() {
        restore_objects();
        restore_refs();
    }

    void state_preserver::retrieve_data() {
        std::vector<uint32_t> refs;
        garbage_collector::get_strong_refs(refs);

        for (uint32_t ref : refs) {
            void* object_ptr = garbage_collector::get_ref_data(ref);
            if (object_ptr == nullptr) {
                continue;
            }

            void* _class = script_engine::get_class_from_object(object_ptr);
            void* assembly = script_engine::get_assembly_from_class(_class);

            if (std::find(m_reloaded_assemblies.begin(), m_reloaded_assemblies.end(), assembly) ==
                m_reloaded_assemblies.end()) {
                continue;
            }

            if (m_data.find(object_ptr) == m_data.end()) {
                m_data.insert(std::make_pair(object_ptr, managed_object_t()));
            }

            std::vector<uint32_t*> ptrs;
            garbage_collector::get_ref_ptrs(ref, ptrs);
            m_data[object_ptr].refs.insert(std::make_pair(ref, ptrs));
        }

        for (const auto& [object_ptr, object_data] : m_data) {
            if (object_data._class.empty()) {
                parse_object(object_ptr);
            }
        }
    }

    bool state_preserver::is_tracked_class(void* _class) {
        class_name_t class_name;
        script_engine::get_class_name(_class, class_name);

        for (void* assembly : m_reloaded_assemblies) {
            if (script_engine::get_class(assembly, class_name) != nullptr) {
                return true;
            }
        }

        return false;
    }

    void state_preserver::parse_object(void* object_ptr) {
        if (m_data.find(object_ptr) == m_data.end()) {
            m_data.insert(std::make_pair(object_ptr, managed_object_t()));
        }

        auto& object_data = m_data[object_ptr];
        void* _class = script_engine::get_class_from_object(object_ptr);

        class_name_t class_name;
        script_engine::get_class_name(_class, class_name);

        object_data._class = script_engine::get_string(class_name);
        object_data.assembly = script_engine::get_assembly_from_class(_class);
        object_data.object = nullptr;

        void* corlib = script_engine::get_mscorlib();
        void* object_class = script_engine::get_class(corlib, "System.Object");

        void* current_type = _class;
        while (current_type != object_class) {
            std::vector<void*> fields;
            script_engine::iterate_fields(current_type, fields);

            for (void* field : fields) {
                managed_object_data_t data;
                data.identifier = field;
                data.field = true;
                data.value_type = false;
                data.data = nullptr;
                data.handle = 0;

                std::string field_name = script_engine::get_field_name(field);
                object_data.data.insert(std::make_pair(field_name, data));

                void* field_value = script_engine::get_field_value(object_ptr, field);
                if (field_value != nullptr) {
                    void* field_type = script_engine::get_field_type(field);
                    parse_object_data(field_value, field_type, object_data.data[field_name]);
                }
            }

            /* im not sure we need properties to be kept - data is always stored in fields
            std::vector<void*> properties;
            script_engine::iterate_properties(current_type, properties);

            for (void* property : properties) {
                static constexpr uint32_t required_accessors =
                    property_accessor_get | property_accessor_set;

                uint32_t accessors = script_engine::get_property_accessors(property);
                if ((accessors & required_accessors) != required_accessors) {
                    continue;
                }

                // todo: check for parameters

                managed_object_data_t data;
                data.identifier = property;
                data.field = false;
                data.value_type = false;
                data.data = nullptr;
                data.handle = 0;

                std::string property_name = script_engine::get_property_name(property);
                object_data.data.insert(std::make_pair(property_name, data));

                void* value = script_engine::get_property_value(object_ptr, property);
                if (value != nullptr) {
                    void* property_type = script_engine::get_property_type(property);
                    parse_object_data(value, property_type, object_data.data[property_name]);
                }
            }
            */

            current_type = script_engine::get_base_class(current_type);
        }
    }

    void state_preserver::parse_object_data(void* value, void* type, managed_object_data_t& data) {
        data.value_type = script_engine::is_value_type(type);
        if (data.value_type) {
            size_t data_size = script_engine::get_type_size(type);
            data.data = malloc(data_size);

            if (data.data == nullptr) {
                throw std::runtime_error("could not allocate memory for state preservation");
            }

            // good enough for now, a simple memcpy
            const void* unboxed = script_engine::unbox_object(value);
            memcpy(data.data, unboxed, data_size);
        } else {
            void* _class = script_engine::get_class_from_object(value);
            void* assembly = script_engine::get_assembly_from_class(_class);

            if (std::find(m_reloaded_assemblies.begin(), m_reloaded_assemblies.end(), assembly) !=
                m_reloaded_assemblies.end()) {
                data.data = value;

                if (m_data.find(value) == m_data.end() || m_data[value]._class.empty()) {
                    parse_object(value);
                }
            } else {
                data.handle = garbage_collector::create_ref(value);
                garbage_collector::add_ref_ptr(data.handle);
            }
        }
    }

    void state_preserver::destroy_objects() {
        for (const auto& [object_ptr, object_data] : m_data) {
            for (const auto& [handle, ptrs] : object_data.refs) {
                garbage_collector::destroy_ref(handle);
            }
        }

        garbage_collector::collect(true);
    }

    bool state_preserver::restore_object(void* object_ptr) {
        if (m_data.find(object_ptr) == m_data.end()) {
            return false;
        }

        auto& data = m_data[object_ptr];
        if (data.object != nullptr) {
            return true;
        }

        void* _class = nullptr;
        for (size_t i = 0; i < script_engine::get_assembly_count(); i++) {
            void* assembly = script_engine::get_assembly(i);
            void* found_class = script_engine::get_class(assembly, data._class);

            if (found_class != nullptr) {
                _class = found_class;
                break;
            }
        }

        if (_class == nullptr) {
            throw std::runtime_error("could not find class: " + data._class);
        }

        data.object = script_engine::alloc_object(_class);
        if (script_engine::get_method(_class, ".ctor") != nullptr) {
            void* constructor = script_engine::get_method(_class, ".ctor()");
            if (constructor != nullptr) {
                script_engine::call_method(data.object, constructor);
            } else {
                throw std::runtime_error("could not find a suitable constructor!");
            }
        } else {
            script_engine::init_object(data.object);
        }

        for (const auto& [name, entry_data] : data.data) {
            std::function<void(void*)> set_data;
            void* identifier = entry_data.identifier;

            if (entry_data.field) {
                set_data = [&](void* value) {
                    script_engine::set_field_value(data.object, identifier, value);
                };
            } else {
                set_data = [&](void* value) {
                    script_engine::set_property_value(data.object, identifier, value);
                };
            }

            if (entry_data.value_type) {
                set_data(entry_data.data);
                free(entry_data.data);
            } else {
                if (entry_data.handle != 0) {
                    void* value = garbage_collector::get_ref_data(entry_data.handle);
                    garbage_collector::destroy_ref(entry_data.handle);

                    set_data(value);
                } else {
                    if (!restore_object(entry_data.data)) {
                        throw std::runtime_error("malformed data!");
                    }

                    void* value = m_data[entry_data.data].object;
                    set_data(value);
                }
            }
        }

        return true;
    }

    void state_preserver::restore_objects() {
        for (const auto& [object_ptr, object_data] : m_data) {
            restore_object(object_ptr);
        }
    }

    void state_preserver::restore_refs() {
        for (const auto& [object_ptr, object_data] : m_data) {
            for (const auto& [original_handle, ptrs] : object_data.refs) {
                uint32_t new_handle = garbage_collector::create_ref(object_ptr);

                for (uint32_t* ptr : ptrs) {
                    *ptr = new_handle;
                    garbage_collector::add_ref_ptr(*ptr);
                }
            }
        }
    }
} // namespace sge
