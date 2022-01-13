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
#include "sge/script/internal_calls.h"
namespace sge {
    struct assembly_t {
        MonoAssembly* assembly;
        MonoImage* image;
    };

    struct script_engine_data_t {
        MonoDomain* root_domain = nullptr;
        std::vector<assembly_t> assemblies;
    };
    static std::unique_ptr<script_engine_data_t> script_engine_data;

    void script_engine::init() {
        if (script_engine_data) {
            throw std::runtime_error("the script engine has already been initialized!");
        }
        script_engine_data = std::make_unique<script_engine_data_t>();

        mono_config_parse(nullptr);
        mono_set_rootdir();

        script_engine_data->root_domain = mono_jit_init("SGE");
        garbage_collector::init();

        load_assembly(fs::current_path() / "assemblies" / "SGE.dll");
        register_internal_script_calls();
    }

    void script_engine::shutdown() {
        if (!script_engine_data) {
            throw std::runtime_error("the script engine is not initialized!");
        }

        garbage_collector::shutdown();

        // todo: close?

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

    size_t script_engine::load_assembly(const fs::path& path) {
        std::string string_path = path.string();
        if (!fs::exists(path)) {
            throw std::runtime_error("assembly does not exist: " + string_path);
        }

        assembly_t assembly;
        MonoImageOpenStatus status;

        assembly.assembly = mono_assembly_open_full(string_path.c_str(), &status, false);
        assembly.image = mono_assembly_get_image(assembly.assembly);

        if (status != MONO_IMAGE_OK) {
            std::string error_message = mono_image_strerror(status);
            throw std::runtime_error("could not open " + string_path + ": " + error_message);
        }

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

    void script_engine::close_assembly(size_t index) {
        if (index >= script_engine_data->assemblies.size() ||
            script_engine_data->assemblies[index].assembly == nullptr) {
            throw std::runtime_error("attempted to close a nonexistent assembly!");
        }

        if (index == 0) {
            throw std::runtime_error("index 0 is reserved!");
        }

        // todo: close?

        script_engine_data->assemblies[index] = { nullptr, nullptr };
    }

    size_t script_engine::get_assembly_count() { return script_engine_data->assemblies.size(); }

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

    void script_engine::iterate_classes(size_t assembly, std::vector<void*>& classes) {
        if (assembly >= script_engine_data->assemblies.size() ||
            script_engine_data->assemblies[assembly].assembly == nullptr) {
            throw std::runtime_error("attempted to iterate over a nonexistent assembly!");
        }

        auto image = script_engine_data->assemblies[assembly].image;
        auto table = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
        int32_t row_count = mono_table_info_get_rows(table);

        classes.clear();
        for (int32_t i = 1; i < row_count; i++) {
            auto _class = mono_class_get(image, (i + 1) | MONO_TOKEN_TYPE_DEF);
            classes.push_back(_class);
        }
    }

    void* script_engine::get_class(size_t assembly, const std::string& name) {
        class_name_t class_name;
        class_name.class_name = name;

        size_t separator_pos = name.find_last_of('.');
        if (separator_pos != std::string::npos) {
            class_name.class_name = name.substr(separator_pos + 1);
            class_name.namespace_name = name.substr(0, separator_pos);
        }

        return get_class(assembly, class_name);
    }

    void* script_engine::get_class(size_t assembly, const class_name_t& name) {
        if (assembly >= script_engine_data->assemblies.size() ||
            script_engine_data->assemblies[assembly].assembly == nullptr) {
            throw std::runtime_error("assembly " + std::to_string(assembly) + " does not exist!");
        }

        const auto& mono_assembly = script_engine_data->assemblies[assembly];
        return mono_class_from_name(mono_assembly.image, name.namespace_name.c_str(),
                                    name.class_name.c_str());
    }

    void* script_engine::get_class_from_object(void* object) {
        auto mono_object = (MonoObject*)object;
        return mono_object_get_class(mono_object);
    }

    void* script_engine::alloc_object(void* _class) {
        auto mono_class = (MonoClass*)_class;

        void* object = mono_object_new(script_engine_data->root_domain, mono_class);
        if (object == nullptr) {
            throw std::runtime_error("could not create object!");
        }

        return object;
    }

    void script_engine::init_object(void* object) {
        auto mono_object = (MonoObject*)object;
        mono_runtime_object_init(mono_object);
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

    static void handle_mono_exception(MonoObject* exception) {
        if (exception != nullptr) {
            spdlog::error("exception was thrown - whoopsie");
        }
    }

    void* script_engine::call_method(void* object, void* method, void** arguments) {
        if (method == nullptr) {
            throw std::runtime_error("attempted to call nullptr!");
        }
        auto mono_method = (MonoMethod*)method;

        MonoObject* exc = nullptr;
        void* returned = mono_runtime_invoke(mono_method, object, arguments, &exc);
        handle_mono_exception(exc);

        return returned;
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
} // namespace sge