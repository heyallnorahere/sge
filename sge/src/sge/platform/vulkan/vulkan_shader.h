/*
   Copyright 2021 Nora Beda

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
#include "sge/renderer/shader.h"
namespace sge {
    class vulkan_shader : public shader {
    public:
        enum class resource_type {
            uniform_buffer,
            storage_buffer,
            image,
            sampler,
            sampled_image,
        };
        struct resource {
            uint32_t set, binding;
            resource_type type;
            size_t size, descriptor_count;
        };
        struct push_constant_range {
            size_t offset, size;
            VkShaderStageFlags stage;
        };
        struct reflection_data {
            std::map<std::string, resource> resources;
            std::vector<push_constant_range> push_constant_ranges;
        };

        static VkShaderStageFlagBits get_shader_stage_flags(shader_stage stage);

        vulkan_shader(const fs::path& path, shader_language language);
        virtual ~vulkan_shader() override;

        virtual void reload() override;
        virtual const fs::path& get_path() override { return this->m_path; }

        const std::vector<VkPipelineShaderStageCreateInfo>& get_pipeline_info() {
            return this->m_pipeline_info;
        }
        const reflection_data& get_reflection_data() { return this->m_reflection_data; }

    private:
        void create();
        void destroy();

        VkShaderModule compile(shader_stage stage, const std::string& source);
        void reflect(const std::vector<uint32_t>& spirv, shader_stage stage);

        fs::path m_path;
        shader_language m_language;

        std::vector<VkPipelineShaderStageCreateInfo> m_pipeline_info;
        reflection_data m_reflection_data;
    };
} // namespace sge