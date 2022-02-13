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
#include "sge/platform/directx/directx_base.h"
#include "sge/platform/directx/directx_shader.h"
#include "sge/renderer/shader_internals.h"
#include "sge/renderer/renderer.h"

#include <d3dcompiler.h>
#include <shaderc/shaderc.hpp>
#include <spirv_hlsl.hpp>

namespace sge {
    static void to_hlsl(std::map<shader_stage, std::string>& source, const fs::path& path) {
        shaderc::Compiler compiler;

        shaderc::CompileOptions options;
        options.SetGenerateDebugInfo();
        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_0);
        options.SetOptimizationLevel(shaderc_optimization_level_performance);

        for (const auto& [stage, glsl] : source) {
            shaderc_shader_kind kind;
            switch (stage) {
            case shader_stage::vertex:
                kind = shaderc_vertex_shader;
                break;
            case shader_stage::fragment:
                kind = shaderc_fragment_shader;
                break;
            default:
                throw std::runtime_error("invalid shader stage!");
            }

            std::string string_path = path.string();
            auto result = compiler.CompileGlslToSpv(glsl, kind, string_path.c_str(), options);

            if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
                throw std::runtime_error("failed to compile to intermediate SPIR-V: " +
                                         result.GetErrorMessage());
            }

            std::vector<uint32_t> spirv(result.cbegin(), result.cend());
            spirv_cross::CompilerHLSL decompiler(std::move(spirv));

            // todo: settings?

            source[stage] = decompiler.compile();
        }
    }

    directx_shader::directx_shader(const fs::path& path, shader_language language) {
        m_path = path;
        m_language = language;

        load();
    }

    void directx_shader::reload() {
        load();
        renderer::on_shader_reloaded(id);
    }

    void directx_shader::load() {
        std::map<shader_stage, std::string> source;
        parse_source(m_path, source);

        if (m_language == shader_language::glsl) {
            to_hlsl(source, m_path);
        }

        uint32_t shader_flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef SGE_DEBUG
        shader_flags |= D3DCOMPILE_DEBUG;
#endif

        std::string string_path = m_path.string();
        m_dxil.clear();
        for (const auto& [stage, stage_source] : source) {

            std::string stage_name;
            switch (stage) {
            case shader_stage::vertex:
                stage_name = "vs";
                break;
            case shader_stage::fragment:
                stage_name = "ps";
                break;
            default:
                throw std::runtime_error("invalid stage name!");
            }

            static const std::string version = "5_1";
            std::string profile = stage_name + "_" + version;

            ComPtr<ID3D10Blob> dxil, error;
            if (FAILED(D3DCompile(stage_source.c_str(), (stage_source.length() + 1) * sizeof(char),
                                  string_path.c_str(), nullptr, nullptr, "main", profile.c_str(),
                                  shader_flags, 0, &dxil, &error))) {
                std::string error_string = (char*)error->GetBufferPointer();
                throw std::runtime_error("failed to compile " + string_path + ": " + error_string);
            }

            m_dxil.insert(std::make_pair(stage, dxil));
        }
    }
} // namespace sge
