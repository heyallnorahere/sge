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
#include "sge/renderer/shader.h"
#ifdef SGE_USE_VULKAN
#include "sge/platform/vulkan/vulkan_base.h"
#include "sge/platform/vulkan/vulkan_shader.h"
#endif
#ifdef SGE_USE_DIRECTX
#include "sge/platform/directx/directx_base.h"
#include "sge/platform/directx/directx_shader.h"
#endif
namespace sge {
    static const std::unordered_map<std::string, shader_stage> stage_map = {
        { "vertex", shader_stage::vertex },
        { "fragment", shader_stage::fragment },
        { "pixel", shader_stage::fragment },
    };

    void shader::parse_source(const fs::path& path, std::map<shader_stage, std::string>& source) {
        std::ifstream file(path);
        if (!file.is_open()) {
            throw std::runtime_error("could not read shader: " + path.string());
        }

        std::string line;
        std::map<shader_stage, std::stringstream> streams;
        std::optional<shader_stage> current_stage;
        static const std::string stage_definition = "#stage ";
        while (std::getline(file, line)) {
            if (line.substr(0, stage_definition.length()) == stage_definition) {
                std::string stage = line.substr(stage_definition.length());

                auto it = stage_map.find(stage);
                if (it == stage_map.end()) {
                    throw std::runtime_error("invalid stage name: " + stage);
                }

                current_stage = it->second;
            } else {
                if (!current_stage.has_value()) {
                    spdlog::warn("{0}: no stage specified, assuming vertex", path.string());
                    current_stage = shader_stage::vertex;
                }

                shader_stage stage = current_stage.value();
                streams[stage] << line << '\n';
            }
        }

        for (const auto& [stage, stream] : streams) {
            source[stage] = stream.str();
        }
    }

    ref<shader> shader::create(const fs::path& path, shader_language language) {
        fs::path filepath = fs::absolute(path);

#ifdef SGE_USE_DIRECTX
        return ref<directx_shader>::create(filepath, language);
#endif

#ifdef SGE_USE_VULKAN
        return ref<vulkan_shader>::create(filepath, language);
#endif

        return nullptr;
    }

    static const std::unordered_map<std::string, shader_language> language_map = {
        { ".hlsl", shader_language::hlsl }, { ".glsl", shader_language::glsl }
    };

    ref<shader> shader::create(const fs::path& path) {
        fs::path extension = path.extension();
        auto it = language_map.find(extension.string());
        if (it == language_map.end()) {
            throw std::runtime_error("cannot determine language from extension: " +
                                     extension.string());
        }

        shader_language language = it->second;
        return create(path, language);
    }

    void shader_library::reload_all() {
        for (const auto& [name, _shader] : m_library) {
            _shader->reload();
        }
    }

    bool shader_library::add(const std::string& name, ref<shader> _shader) {
        if (m_library.find(name) != m_library.end()) {
            return false;
        }

        m_library.insert(std::make_pair(name, _shader));
        return true;
    }

    ref<shader> shader_library::add(const std::string& name, const fs::path& path) {
        if (m_library.find(name) != m_library.end() || !fs::exists(path)) {
            return nullptr;
        }

        auto _shader = shader::create(path);
        add(name, _shader);
        return _shader;
    }

    ref<shader> shader_library::add(const std::string& name, const fs::path& path,
                                    shader_language language) {
        if (m_library.find(name) != m_library.end() || !fs::exists(path)) {
            return nullptr;
        }

        auto _shader = shader::create(path, language);
        add(name, _shader);
        return _shader;
    }

    ref<shader> shader_library::get(const std::string& name) {
        ref<shader> _shader;
        if (m_library.find(name) != m_library.end()) {
            _shader = m_library[name];
        }
        return _shader;
    }
} // namespace sge