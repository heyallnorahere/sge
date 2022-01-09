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
#include "sge/platform/vulkan/vulkan_base.h"
#include "sge/platform/vulkan/vulkan_shader.h"
#include "sge/platform/vulkan/vulkan_context.h"
#include "sge/renderer/renderer.h"
#include <shaderc/shaderc.hpp>
#include <spirv_glsl.hpp>
namespace sge {
    class file_finder : public shaderc::CompileOptions::IncluderInterface {
    public:
        virtual shaderc_include_result* GetInclude(const char* requested_source,
                                                   shaderc_include_type type,
                                                   const char* requesting_source,
                                                   size_t include_depth) {
            // get c++17 paths
            fs::path requested_path = requested_source;
            fs::path requesting_path = requesting_source;

            // sort out paths
            if (!requesting_path.has_parent_path()) {
                requesting_path = fs::absolute(requesting_path);
            }
            if (type != shaderc_include_type_standard) {
                requested_path = requesting_path.parent_path() / requested_path;
            }

            // read data
            auto file_info = new included_file_info;
            file_info->path = requested_path.string();
            {
                if (!fs::exists(requested_path)) {
                    throw std::runtime_error(requested_path.string() + " does not exist!");
                }

                std::ifstream file(requested_path);
                std::string line;
                while (std::getline(file, line)) {
                    file_info->content += line + '\n';
                }
                file.close();
            }

            // return result
            auto result = new shaderc_include_result;
            result->user_data = file_info;
            result->content = file_info->content.c_str();
            result->content_length = file_info->content.length();
            result->source_name = file_info->path.c_str();
            result->source_name_length = file_info->path.length();
            return result;
        }

        virtual void ReleaseInclude(shaderc_include_result* data) {
            delete (included_file_info*)data->user_data;
            delete data;
        }

    private:
        struct included_file_info {
            std::string content, path;
        };
    };

    VkShaderStageFlagBits vulkan_shader::get_shader_stage_flags(shader_stage stage) {
        switch (stage) {
        case shader_stage::vertex:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case shader_stage::fragment:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        default:
            throw std::runtime_error("invalid shader stage!");
            return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
        }
    }

    vulkan_shader::vulkan_shader(const fs::path& path, shader_language language) {
        this->m_path = path;
        this->m_language = language;

        this->create();
    }

    vulkan_shader::~vulkan_shader() { this->destroy(); }

    void vulkan_shader::reload() {
        this->destroy();
        this->create();
        renderer::on_shader_reloaded(this);
    }

    void vulkan_shader::create() {
        std::map<shader_stage, std::string> sources;
        parse_source(this->m_path, sources);

        for (const auto& [stage, source] : sources) {
            auto stage_info = vk_init<VkPipelineShaderStageCreateInfo>(
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);

            stage_info.pName = "main";
            stage_info.stage = get_shader_stage_flags(stage);
            stage_info.module = this->compile(stage, source);

            this->m_pipeline_info.push_back(stage_info);
        }

        { 
            size_t ubo_count = 0;
            size_t ssbo_count = 0;
            size_t image_count = 0;
            size_t sampler_count = 0;
            size_t combined_image_sampler_count = 0;

            for (const auto& [name, data] : m_reflection_data.resources) {
                switch (data.type) {
                case resource_type::uniform_buffer:
                    ubo_count++;
                    break;
                case resource_type::storage_buffer:
                    ssbo_count++;
                    break;
                case resource_type::image:
                    image_count++;
                    break;
                case resource_type::sampler:
                    sampler_count++;
                    break;
                case resource_type::sampled_image:
                    combined_image_sampler_count++;
                    break;
                default:
                    throw std::runtime_error("invalid resource type!");
                }
            }

            spdlog::info("{0} reflection results:", m_path.string());
            spdlog::info("{0} uniform buffer(s)", ubo_count);
            spdlog::info("{0} storage buffer(s)", ssbo_count);
            spdlog::info("{0} separate image set(s)", image_count);
            spdlog::info("{0} separate sampler set(s)", sampler_count);
            spdlog::info("{0} combined image sampler set(s)", combined_image_sampler_count);
        }
    }

    void vulkan_shader::destroy() {
        VkDevice device = vulkan_context::get().get_device().get();

        for (const auto& stage_info : this->m_pipeline_info) {
            vkDestroyShaderModule(device, stage_info.module, nullptr);
        }
        this->m_pipeline_info.clear();

        this->m_reflection_data.resources.clear();
        this->m_reflection_data.push_constant_buffer = push_constant_range();
    }

    static void compile_shader(shader_stage stage, const std::string& source,
                               shader_language language, const fs::path& path,
                               std::vector<uint32_t>& spirv) {
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;

        // maybe change for dist builds
        options.SetOptimizationLevel(shaderc_optimization_level_zero);
        options.SetGenerateDebugInfo();

        shaderc_source_language source_language;
        switch (language) {
        case shader_language::glsl:
            source_language = shaderc_source_language_glsl;
            break;
        case shader_language::hlsl:
            source_language = shaderc_source_language_hlsl;
            break;
        default:
            throw std::runtime_error("invalid shader language!");
        }
        options.SetSourceLanguage(source_language);

        uint32_t vulkan_version = vulkan_context::get().get_vulkan_version();
        options.SetTargetEnvironment(shaderc_target_env_vulkan, vulkan_version);

        std::unique_ptr<shaderc::CompileOptions::IncluderInterface> includer(new file_finder);
        options.SetIncluder(std::move(includer));

        shaderc_shader_kind kind;
        std::string stage_name;
        switch (stage) {
        case shader_stage::vertex:
            kind = shaderc_vertex_shader;
            stage_name = "vertex";
            break;
        case shader_stage::fragment:
            kind = shaderc_fragment_shader;
            stage_name = "fragment";
            break;
        default:
            throw std::runtime_error("invalid shader stage!");
        }

        std::string string_path = path.string();
        auto result = compiler.CompileGlslToSpv(source, kind, string_path.c_str(), "main", options);

        if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
            throw std::runtime_error("could not compile " + stage_name + " shader " + string_path +
                                     ": " + result.GetErrorMessage());
        }

        spirv = std::vector<uint32_t>(result.cbegin(), result.cend());
    }

    VkShaderModule vulkan_shader::compile(shader_stage stage, const std::string& source) {
        std::vector<uint32_t> spirv;
        compile_shader(stage, source, this->m_language, this->m_path, spirv);
        this->reflect(spirv, stage);

        auto create_info =
            vk_init<VkShaderModuleCreateInfo>(VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);

        create_info.pCode = spirv.data();
        create_info.codeSize = spirv.size() * sizeof(uint32_t);

        VkDevice device = vulkan_context::get().get_device().get();
        VkShaderModule module;
        VkResult result = vkCreateShaderModule(device, &create_info, nullptr, &module);
        check_vk_result(result);
        return module;
    }

    static void map_resources(const spirv_cross::SmallVector<spirv_cross::Resource>& resources,
                              vulkan_shader::reflection_data& reflection_data, shader_stage stage,
                              vulkan_shader::resource_type type, spirv_cross::Compiler& compiler) {
        for (const auto& resource : resources) {
            vulkan_shader::resource data;
            data.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            data.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
            data.type = type;
            data.stage = stage;

            const auto& type = compiler.get_type(resource.type_id);
            if (type.array.empty()) {
                data.descriptor_count = 1;
            } else {
                data.descriptor_count = type.array[0];
            }

            if (type.basetype == spirv_cross::SPIRType::BaseType::Struct) {
                data.size = compiler.get_declared_struct_size(type);
            }

            std::string name = compiler.get_name(resource.id);
            if (reflection_data.resources.find(name) != reflection_data.resources.end()) {
                throw std::runtime_error("a resource named " + name + " has already been defined!");
            }
            reflection_data.resources.insert(std::make_pair(name, data));
        }
    }

    void vulkan_shader::reflect(const std::vector<uint32_t>& spirv, shader_stage stage) {
        spirv_cross::Compiler compiler(std::move(spirv));
        auto resources = compiler.get_shader_resources();

        map_resources(resources.uniform_buffers, this->m_reflection_data, stage,
                      resource_type::uniform_buffer, compiler);
        map_resources(resources.storage_buffers, this->m_reflection_data, stage,
                      resource_type::storage_buffer, compiler);
        map_resources(resources.sampled_images, this->m_reflection_data, stage,
                      resource_type::sampled_image, compiler);
        map_resources(resources.separate_images, this->m_reflection_data, stage,
                      resource_type::image, compiler);
        map_resources(resources.separate_samplers, this->m_reflection_data, stage,
                      resource_type::sampler, compiler);

        for (const auto& spirv_resource : resources.push_constant_buffers) {
            const auto& spirv_type = compiler.get_type(spirv_resource.type_id);
            size_t size = compiler.get_declared_struct_size(spirv_type);
            this->m_reflection_data.push_constant_buffer.size += size;

            VkShaderStageFlagBits stage_flags = get_shader_stage_flags(stage);
            this->m_reflection_data.push_constant_buffer.stage |= stage_flags;
        }
    }
} // namespace sge