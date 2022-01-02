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
#include "sge/renderer/pipeline.h"
#include "sge/platform/vulkan/vulkan_uniform_buffer.h"
namespace sge {
    class vulkan_pipeline : public pipeline {
    public:
        vulkan_pipeline(const pipeline_spec& spec);
        virtual ~vulkan_pipeline() override;

        virtual void invalidate() override;

        virtual pipeline_spec& get_spec() override { return this->m_spec; }
        virtual const pipeline_spec& get_spec() const override { return this->m_spec; }

        virtual void set_uniform_buffer(ref<uniform_buffer> ubo, uint32_t binding) override;

        VkPipeline get_pipeline() { return this->m_pipeline; }
        VkPipelineLayout get_pipeline_layout() { return this->m_layout; }
        void get_descriptor_sets(std::map<uint32_t, std::vector<VkDescriptorSet>>& sets);

    private:
        struct descriptor_set_t {
            VkDescriptorSetLayout layout;
            std::vector<VkDescriptorSet> sets;
        };

        struct descriptor_sets_t {
            VkDescriptorPool pool;
            std::map<uint32_t, descriptor_set_t> sets;
        };

        struct descriptor_set_binding_t {
            ref<vulkan_uniform_buffer> ubo;
        };

        void create();
        void destroy();

        void create_descriptor_sets();
        void create_pipeline();

        void write(ref<vulkan_uniform_buffer> ubo, uint32_t binding,
                   std::vector<VkWriteDescriptorSet>& writes,
                   std::vector<VkDescriptorBufferInfo>& buffer_info);

        VkPipeline m_pipeline;
        VkPipelineLayout m_layout;

        pipeline_spec m_spec;
        descriptor_sets_t m_descriptor_sets;

        std::map<uint32_t, descriptor_set_binding_t> m_bindings;
    };
} // namespace sge