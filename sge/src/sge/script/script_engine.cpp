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
#include <mono/metadata/mono-config.h>
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

        load_assembly(fs::current_path() / "assemblies" / "SGE.dll");
    }

    void script_engine::shutdown() {
        if (!script_engine_data) {
            throw std::runtime_error("the script engine is not initialized!");
        }

        // todo: close?

        mono_jit_cleanup(script_engine_data->root_domain);
        script_engine_data.reset();
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

    void* script_engine::get_class(size_t assembly, const std::string& name) {
        std::string class_name = name;
        std::string namespace_name;

        size_t separator_pos = name.find_last_of('.');
        if (separator_pos != std::string::npos) {
            class_name = name.substr(separator_pos + 1);
            namespace_name = name.substr(0, separator_pos);
        }

        return get_class(assembly, namespace_name, class_name);
    }

    void* script_engine::get_class(size_t assembly, const std::string& namespace_name,
                                   const std::string& name) {
        if (assembly >= script_engine_data->assemblies.size() ||
            script_engine_data->assemblies[assembly].assembly == nullptr) {
            throw std::runtime_error("assembly " + std::to_string(assembly) + " does not exist!");
        }

        const auto& mono_assembly = script_engine_data->assemblies[assembly];
        return mono_class_from_name(mono_assembly.image, namespace_name.c_str(), name.c_str());
    }

    void* script_engine::get_class(void* parameter_type) {
        auto reflection_type = (MonoReflectionType*)parameter_type;
        auto type = mono_reflection_type_get_type(reflection_type);
        return mono_type_get_class(type);
    }

    void* script_engine::get_parameter_type(void* _class) {
        auto mono_class = (MonoClass*)_class;
        auto type = mono_class_get_type(mono_class);
        return mono_type_get_object(script_engine_data->root_domain, type);
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
} // namespace sge