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

#pragma once
namespace sge {
    struct class_name_t {
        std::string namespace_name, class_name;
    };

    struct method_parameter_t {
        std::string name;
        void* type;
    };

    enum property_accessor_flags {
        property_accessor_none = 0x0,
        property_accessor_get = 0x1,
        property_accessor_set = 0x2
    };

    enum member_visibility_flags {
        member_visibility_flags_none = 0x0,
        member_visibility_flags_public = 0x1,
        member_visibility_flags_protected = 0x2,
        member_visibility_flags_private = 0x4,
        member_visibility_flags_internal = 0x8,
        member_visibility_flags_static = 0x10
    };

    class script_engine {
    public:
        script_engine() = delete;

        static void init();
        static void shutdown();
        static void register_internal_call(const std::string& name, const void* callback);

        static size_t load_assembly(const fs::path& path);
        static size_t get_assembly_count();

        static std::string get_string(const class_name_t& class_name);
        static void get_class_name(void* _class, class_name_t& name);

        static void iterate_classes(size_t assembly, std::vector<void*>& classes);
        static void* get_class(size_t assembly, const std::string& name);
        static void* get_class(size_t assembly, const class_name_t& name);
        static void* get_class_from_object(void* object);

        static void* alloc_object(void* _class);
        static void init_object(void* object);
        static const void* unbox_object(void* object);

        template <typename T>
        static const T& unbox_object(void* object) {
            const void* ptr = unbox_object(object);
            return *(const T*)ptr;
        }

        static void* get_method(void* _class, const std::string& name);
        static std::string get_method_name(void* method);
        static void* get_method_return_type(void* method);
        static uint32_t get_method_visibility(void* method);
        static void get_method_parameters(void* method,
                                          std::vector<method_parameter_t>& parameters);
        static void iterate_methods(void* _class, std::vector<void*>& methods);

        static void* call_method(void* object, void* method, void** arguments = nullptr);
        static void handle_exception(void* exception);

        template <typename... Args>
        static void* call_method(void* object, void* method, Args*... args) {
            std::vector<void*> args_vector = { std::forward<Args*>(args)... };
            void** arguments = args_vector.empty() ? nullptr : args_vector.data();

            return call_method(object, method, arguments);
        }

        static void* get_property(void* _class, const std::string& name);
        static void iterate_properties(void* _class, std::vector<void*>& properties);

        static std::string get_property_name(void* property);
        static void* get_property_type(void* property);
        static uint32_t get_property_accessors(void* property);
        static uint32_t get_property_visibility(void* property);

        static void* get_field(void* _class, const std::string& name);
        static void iterate_fields(void* _class, std::vector<void*>& fields);

        static std::string get_field_name(void* field);
        static void* get_field_type(void* field);

        static void* to_managed_string(const std::string& native_string);
        static std::string from_managed_string(void* managed_string);

        static void* to_reflection_type(void* _class);
        static void* from_reflection_type(void* reflection_type);

        static void* to_reflection_property(void* property);

        static void* get_property_value(void* object, void* property, void** arguments = nullptr);
        static void set_property_value(void* object, void* property, void** arguments);

        template <typename... Args>
        static void* get_property_value(void* object, void* property, Args*... args) {
            std::vector<void*> args_vector = { std::forward<Args*>(args)... };
            void** arguments = args_vector.empty() ? nullptr : args_vector.data();

            return get_property_value(object, property, arguments);
        }

        template <typename... Args>
        static void set_property_value(void* object, void* property, void* value, Args*... args) {
            std::vector<void*> args_vector = { std::forward<Args*>(args)... };
            args_vector.insert(args_vector.begin(), value);

            set_property_value(object, property, args_vector.data());
        }

        static void* get_field_value(void* object, void* field);
        static void set_field_value(void* object, void* field, void* value);
    };
} // namespace sge