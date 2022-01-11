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
#include <mono/metadata/mono-config.h>
namespace sge {
    struct script_engine_data_t {
        MonoDomain* root_domain = nullptr;
        std::vector<MonoAssembly*> assemblies;
    };
    static std::unique_ptr<script_engine_data_t> script_engine_data;

    void script_engine::init() {
        if (script_engine_data) {
            throw std::runtime_error("the script engine has already been initialized!");
        }
        script_engine_data = std::make_unique<script_engine_data_t>();

        mono_config_parse(nullptr);
        mono_set_assemblies_path(MONO_ASSEMBLY_DIRECTORY);

        script_engine_data->root_domain = mono_jit_init("SGE");

        // todo: load core SGE assembly
    }

    void script_engine::shutdown() {
        if (!script_engine_data) {
            throw std::runtime_error("the script engine is not initialized!");
        }

        for (auto assembly : script_engine_data->assemblies) {
            mono_assembly_close(assembly);
        }

        mono_jit_cleanup(script_engine_data->root_domain);
        script_engine_data.reset();
    }

    static std::optional<size_t> find_viable_assembly_index() {
        std::optional<size_t> index;

        for (size_t i = 0; i < script_engine_data->assemblies.size(); i++) {
            MonoAssembly* assembly = script_engine_data->assemblies[i];

            if (assembly == nullptr) {
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

        MonoImageOpenStatus status;
        MonoAssembly* assembly = mono_assembly_open_full(string_path.c_str(), &status, false);

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
            script_engine_data->assemblies[index] == nullptr) {
            throw std::runtime_error("attempted to close a nonexistent assembly!");
        }

        if (index == 0) {
            throw std::runtime_error("index 0 is reserved!");
        }

        MonoAssembly* assembly = script_engine_data->assemblies[index];
        script_engine_data->assemblies[index] = nullptr;

        mono_assembly_close(assembly);
    }
} // namespace sge