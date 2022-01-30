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
#include "sge/script/mono_include.h"
#include "sge/script/garbage_collector.h"
#include "sge/script/script_helpers.h"

// todo(nora): don't use mono-private-unstable.h
#include <mono/jit/mono-private-unstable.h>

namespace sge {
    struct assembly_t {
        MonoAssembly* assembly = nullptr;
        MonoImage* image = nullptr;
        fs::path path;
    };

    struct script_engine_data_t {
        MonoDomain* root_domain = nullptr;
        std::vector<assembly_t> assemblies;
    };
    static std::unique_ptr<script_engine_data_t> script_engine_data;

    static std::string build_tpa_list() {
        fs::path assembly_dir = fs::absolute(fs::current_path() / "assets" / "dotnet");
        std::string tpa_list;

#ifdef SGE_PLATFORM_WINDOWS
        static constexpr char separator = ';';
#else
        static constexpr char separator = ':';
#endif

        for (const auto& entry : fs::directory_iterator(assembly_dir)) {
            if (entry.is_directory() || entry.path().extension() != ".dll") {
                continue;
            }

            tpa_list += fs::absolute(entry.path()).string() + separator;
        }

#ifdef SGE_PLATFORM_WINDOWS
        static const fs::path corlib_platform = "windows";
#else
        static const fs::path corlib_platform = "unix";
#endif

        fs::path corlib_path = assembly_dir / corlib_platform / "System.Private.CoreLib.dll";
        tpa_list += fs::absolute(corlib_path).string();

        return tpa_list;
    }

    void script_engine::init() {
        if (script_engine_data) {
            throw std::runtime_error("the script engine has already been initialized!");
        }
        script_engine_data = std::make_unique<script_engine_data_t>();

        {
            std::unordered_map<std::string, std::string> properties;
            properties["TRUSTED_PLATFORM_ASSEMBLIES"] = build_tpa_list();

            std::vector<const char*> keys, values;
            for (const auto& [key, value] : properties) {
                keys.push_back(key.c_str());
                values.push_back(value.c_str());
            }

            if (monovm_initialize(properties.size(), keys.data(), values.data()) != 0) {
                throw std::runtime_error("could not initialize the mono VM!");
            }
        }

        script_engine_data->root_domain = mono_jit_init("SGE");
        garbage_collector::init();

        load_assembly(fs::current_path() / "assemblies" / "SGE.Scriptcore.dll");
        register_internal_script_calls();

        void* helpers_class = get_class(script_engine_data->assemblies[0].image, "SGE.Helpers");
        script_helpers::init(helpers_class);
    }

    void script_engine::shutdown() {
        if (!script_engine_data) {
            throw std::runtime_error("the script engine is not initialized!");
        }

        garbage_collector::shutdown();

        // todo: close assemblies?

        mono_jit_cleanup(script_engine_data->root_domain);
        script_engine_data.reset();
    }

    void script_engine::register_internal_call(const std::string& name, const void* callback) {
        mono_add_internal_call(name.c_str(), callback);
    }

    static std::optional<size_t> find_viable_assembly_index() {
        std::optional<size_t> index;

        for (size_t i = 0; i < script_engine_data->assemblies.size(); i++) {
            const auto& assembly = script_engine_data->assemblies[i];

            if (assembly.assembly == nullptr) {
                index = i;
                break;
            }
        }

        return index;
    }

    std::optional<size_t> script_engine::load_assembly(const fs::path& path) {
        std::string string_path = path.string();
        for (size_t i = 0; i < script_engine_data->assemblies.size(); i++) {
            if (script_engine_data->assemblies[i].path == path) {
                spdlog::warn("attempted to load {0} more than once", string_path);
                return i;
            }
        }

        std::optional<size_t> assembly_index;
        if (!fs::exists(path)) {
            spdlog::warn("assembly does not exist: {0}", string_path);
        } else {
            assembly_t assembly;
            MonoImageOpenStatus status;

            assembly.assembly = mono_assembly_open_full(string_path.c_str(), &status, false);
            assembly.image = mono_assembly_get_image(assembly.assembly);

            if (status != MONO_IMAGE_OK) {
                std::string error_message = mono_image_strerror(status);
                spdlog::warn("could not open {0}: ", string_path, error_message);
            } else {
                auto found_index = find_viable_assembly_index();
                if (found_index.has_value()) {
                    size_t index = found_index.value();
                    script_engine_data->assemblies[index] = assembly;
                    return index;
                } else {
                    size_t index = script_engine_data->assemblies.size();
                    script_engine_data->assemblies.push_back(assembly);
                    return index;
                }
            }
        }

        return assembly_index;
    }

    bool script_engine::unload_assembly(size_t index) {
        if (index < script_engine_data->assemblies.size()) {
            auto& data = script_engine_data->assemblies[index];

            if (data.assembly != nullptr) {
                garbage_collector::collect();
                data.assembly = nullptr;
                return true;
            }
        }

        return false;
    }

    std::optional<size_t> script_engine::reload_assembly(size_t index) {
        std::optional<size_t> new_index;

        if (index < script_engine_data->assemblies.size()) {
            auto& data = script_engine_data->assemblies[index];

            if (data.assembly != nullptr && !data.path.empty()) {
                fs::path path = data.path;
                unload_assembly(index);

                new_index = load_assembly(path);
            }
        }

        return new_index;
    }

    size_t script_engine::get_assembly_count() { return script_engine_data->assemblies.size(); }

    fs::path script_engine::get_assembly_path(size_t index) {
        fs::path assembly_path;

        if (index < script_engine_data->assemblies.size()) {
            assembly_path = script_engine_data->assemblies[index].path;
        }

        return assembly_path;
    }

    void* script_engine::get_assembly(size_t index) {
        if (index >= script_engine_data->assemblies.size()) {
            return nullptr;
        }

        return script_engine_data->assemblies[index].image;
    }

    void* script_engine::get_mscorlib() { return mono_get_corlib(); }

    std::string script_engine::get_string(const class_name_t& class_name) {
        if (!class_name.namespace_name.empty()) {
            return class_name.namespace_name + "." + class_name.class_name;
        } else {
            return class_name.class_name;
        }
    }

    void script_engine::get_class_name(void* _class, class_name_t& name) {
        auto mono_class = (MonoClass*)_class;

        name.namespace_name = mono_class_get_namespace(mono_class);
        name.class_name = mono_class_get_name(mono_class);
    }

    void script_engine::iterate_classes(void* assembly, std::vector<void*>& classes) {
        auto image = (MonoImage*)assembly;
        auto table = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
        int32_t row_count = mono_table_info_get_rows(table);

        classes.clear();
        for (int32_t i = 1; i < row_count; i++) {
            auto _class = mono_class_get(image, (i + 1) | MONO_TOKEN_TYPE_DEF);
            classes.push_back(_class);
        }
    }

    void* script_engine::get_class(void* assembly, const std::string& name) {
        class_name_t class_name;
        class_name.class_name = name;

        size_t separator_pos = name.find_last_of('.');
        if (separator_pos != std::string::npos) {
            class_name.class_name = name.substr(separator_pos + 1);
            class_name.namespace_name = name.substr(0, separator_pos);
        }

        return get_class(assembly, class_name);
    }

    void* script_engine::get_class(void* assembly, const class_name_t& name) {
        auto image = (MonoImage*)assembly;
        return mono_class_from_name(image, name.namespace_name.c_str(), name.class_name.c_str());
    }

    void* script_engine::get_class_from_object(void* object) {
        auto mono_object = (MonoObject*)object;
        return mono_object_get_class(mono_object);
    }

    bool script_engine::is_value_type(void* _class) {
        auto mono_class = (MonoClass*)_class;
        return mono_class_is_valuetype(mono_class);
    }

    size_t script_engine::get_type_size(void* _class) {
        auto mono_class = (MonoClass*)_class;
        return (size_t)mono_class_data_size(mono_class);
    }

    void* script_engine::alloc_object(void* _class) {
        auto mono_class = (MonoClass*)_class;

        void* object = mono_object_new(script_engine_data->root_domain, mono_class);
        if (object == nullptr) {
            throw std::runtime_error("could not create object!");
        }

        return object;
    }

    void* script_engine::clone_object(void* original) {
        if (original == nullptr) {
            return nullptr;
        }

        return mono_object_clone((MonoObject*)original);
    }

    void script_engine::init_object(void* object) {
        auto mono_object = (MonoObject*)object;
        mono_runtime_object_init(mono_object);
    }

    const void* script_engine::unbox_object(void* object) {
        auto mono_object = (MonoObject*)object;
        return mono_object_unbox(mono_object);
    }

    void* script_engine::get_method(void* _class, const std::string& name) {
        std::string method_desc = "*:" + name;
        auto mono_desc = mono_method_desc_new(method_desc.c_str(), false);

        auto mono_class = (MonoClass*)_class;
        return mono_method_desc_search_in_class(mono_desc, mono_class);
    }

    std::string script_engine::get_method_name(void* method) {
        auto mono_method = (MonoMethod*)method;
        return mono_method_get_name(mono_method);
    }

    void* script_engine::get_method_return_type(void* method) {
        auto mono_method = (MonoMethod*)method;
        auto signature = mono_method_signature(mono_method);

        auto return_type = mono_signature_get_return_type(signature);
        return mono_class_from_mono_type(return_type);
    }

    static uint32_t get_visibility_from_flags(uint32_t flags) {
        uint32_t visibility = member_visibility_flags_none;

        switch (flags & MONO_METHOD_ATTR_ACCESS_MASK) {
        case MONO_METHOD_ATTR_PUBLIC:
            visibility |= member_visibility_flags_public;
            break;
        case MONO_METHOD_ATTR_FAMILY:
            visibility |= member_visibility_flags_protected;
            break;
        case MONO_METHOD_ATTR_PRIVATE:
            visibility |= member_visibility_flags_private;
            break;
        case MONO_METHOD_ATTR_ASSEM:
            visibility |= member_visibility_flags_internal;
            break;
        default:
            throw std::runtime_error("invalid access attribute!");
        }

        if ((flags & MONO_METHOD_ATTR_STATIC) != 0) {
            visibility |= member_visibility_flags_static;
        }

        return visibility;
    }

    uint32_t script_engine::get_method_visibility(void* method) {
        uint32_t visibility = member_visibility_flags_none;

        auto mono_method = (MonoMethod*)method;
        uint32_t flags = mono_method_get_flags(mono_method, nullptr);

        return get_visibility_from_flags(flags);
    }

    void script_engine::get_method_parameters(void* method,
                                              std::vector<method_parameter_t>& parameters) {
        auto mono_method = (MonoMethod*)method;
        auto signature = mono_method_signature(mono_method);

        uint32_t param_count = mono_signature_get_param_count(signature);
        auto names = (const char**)malloc(sizeof(size_t) * param_count);
        mono_method_get_param_names(mono_method, names);

        MonoType* type = nullptr;
        void* iterator = nullptr;

        parameters.clear();
        size_t parameter_index = 0;
        while ((type = mono_signature_get_params(signature, &iterator)) != nullptr) {
            method_parameter_t parameter;
            parameter.name = names[parameter_index];
            parameter.type = mono_class_from_mono_type(type);
            parameters.push_back(parameter);

            parameter_index++;
        }

        free(names);
    }

    void script_engine::iterate_methods(void* _class, std::vector<void*>& methods) {
        auto mono_class = (MonoClass*)_class;

        MonoMethod* method = nullptr;
        void* iterator = nullptr;

        methods.clear();
        while ((method = mono_class_get_methods(mono_class, &iterator)) != nullptr) {
            methods.push_back(method);
        }
    }

    void* script_engine::call_method(void* object, void* method, void** arguments) {
        if (method == nullptr) {
            throw std::runtime_error("attempted to call nullptr!");
        }
        auto mono_method = (MonoMethod*)method;

        MonoObject* exc = nullptr;
        void* returned = mono_runtime_invoke(mono_method, object, arguments, &exc);
        handle_exception(exc);

        return returned;
    }

    void script_engine::handle_exception(void* exception) {
        if (exception != nullptr) {
            void* exc_class = get_class_from_object(exception);

            class_name_t class_name;
            get_class_name(exc_class, class_name);
            std::string exception_name = get_string(class_name);

            void* property = get_property(exc_class, "Message");
            void* value = get_property_value(exception, property);
            std::string message = from_managed_string(value);

            property = get_property(exc_class, "Source");
            value = get_property_value(exception, property);
            std::string source = "none";
            if (value != nullptr) {
                source = from_managed_string(value);
            }

            property = get_property(exc_class, "StackTrace");
            value = get_property_value(exception, property);
            std::string stack_trace = "none";
            if (value != nullptr) {
                stack_trace = from_managed_string(value);
            }

            spdlog::error("{0} thrown: {1}\n\tsource: {2}\n\tstack trace: {3}", exception_name,
                          message, source, stack_trace);
        }
    }

    void* script_engine::get_property(void* _class, const std::string& name) {
        auto mono_class = (MonoClass*)_class;
        return mono_class_get_property_from_name(mono_class, name.c_str());
    }

    void script_engine::iterate_properties(void* _class, std::vector<void*>& properties) {
        auto mono_class = (MonoClass*)_class;

        MonoProperty* property = nullptr;
        void* iterator = nullptr;

        properties.clear();
        while ((property = mono_class_get_properties(mono_class, &iterator)) != nullptr) {
            properties.push_back(property);
        }
    }

    std::string script_engine::get_property_name(void* property) {
        auto mono_property = (MonoProperty*)property;
        return mono_property_get_name(mono_property);
    }

    void* script_engine::get_property_type(void* property) {
        auto mono_property = (MonoProperty*)property;

        auto method = mono_property_get_get_method(mono_property);
        if (method != nullptr) {
            return get_method_return_type(method);
        }

        method = mono_property_get_set_method(mono_property);
        if (method != nullptr) {
            void* iterator = nullptr;
            auto signature = mono_method_signature(method);

            auto type = mono_signature_get_params(signature, &iterator);
            return mono_class_from_mono_type(type);
        }

        return nullptr;
    }

    uint32_t script_engine::get_property_accessors(void* property) {
        auto mono_property = (MonoProperty*)property;
        uint32_t accessors = property_accessor_none;

        auto method = mono_property_get_get_method(mono_property);
        if (method != nullptr) {
            accessors |= property_accessor_get;
        }

        method = mono_property_get_set_method(mono_property);
        if (method != nullptr) {
            accessors |= property_accessor_set;
        }

        return accessors;
    }

    uint32_t script_engine::get_property_visibility(void* property) {
        auto mono_property = (MonoProperty*)property;

        MonoMethod* property_method = mono_property_get_get_method(mono_property);
        if (property_method == nullptr) {
            property_method = mono_property_get_set_method(mono_property);
        }

        return get_method_visibility(property_method);
    }

    void* script_engine::get_field(void* _class, const std::string& name) {
        auto mono_class = (MonoClass*)_class;
        return mono_class_get_field_from_name(mono_class, name.c_str());
    }

    void script_engine::iterate_fields(void* _class, std::vector<void*>& fields) {
        auto mono_class = (MonoClass*)_class;

        MonoClassField* field = nullptr;
        void* iterator = nullptr;

        fields.clear();
        while ((field = mono_class_get_fields(mono_class, &iterator)) != nullptr) {
            fields.push_back(field);
        }
    }

    std::string script_engine::get_field_name(void* field) {
        auto mono_field = (MonoClassField*)field;
        return mono_field_get_name(mono_field);
    }

    void* script_engine::get_field_type(void* field) {
        auto mono_field = (MonoClassField*)field;
        auto type = mono_field_get_type(mono_field);
        return mono_class_from_mono_type(type);
    }

    void* script_engine::to_managed_string(const std::string& native_string) {
        return mono_string_new(script_engine_data->root_domain, native_string.c_str());
    }

    std::string script_engine::from_managed_string(void* managed_string) {
        auto mono_string = (MonoString*)managed_string;
        return mono_string_to_utf8(mono_string);
    }

    void* script_engine::to_reflection_type(void* _class) {
        auto mono_class = (MonoClass*)_class;
        auto type = mono_class_get_type(mono_class);
        return mono_type_get_object(script_engine_data->root_domain, type);
    }

    void* script_engine::from_reflection_type(void* reflection_type) {
        auto mono_object = (MonoReflectionType*)reflection_type;
        auto type = mono_reflection_type_get_type(mono_object);
        return mono_class_from_mono_type(type);
    }

    void* script_engine::to_reflection_property(void* property) {
        auto mono_property = (MonoProperty*)property;
        auto mono_class = mono_property_get_parent(mono_property);

        return mono_property_get_object(script_engine_data->root_domain, mono_class, mono_property);
    }

    void* script_engine::get_property_value(void* object, void* property, void** arguments) {
        auto mono_property = (MonoProperty*)property;

        MonoObject* exc = nullptr;
        void* value = mono_property_get_value(mono_property, object, arguments, &exc);
        handle_exception(exc);

        return value;
    }

    void script_engine::set_property_value(void* object, void* property, void** arguments) {
        auto mono_property = (MonoProperty*)property;

        MonoObject* exc = nullptr;
        mono_property_set_value(mono_property, object, arguments, &exc);
        handle_exception(exc);
    }

    void* script_engine::get_field_value(void* object, void* field) {
        auto mono_object = (MonoObject*)object;
        auto mono_field = (MonoClassField*)field;

        return mono_field_get_value_object(script_engine_data->root_domain, mono_field,
                                           mono_object);
    }

    void script_engine::set_field_value(void* object, void* field, void* value) {
        auto mono_object = (MonoObject*)object;
        auto mono_field = (MonoClassField*)field;

        mono_field_set_value(mono_object, mono_field, value);
    }
} // namespace sge
