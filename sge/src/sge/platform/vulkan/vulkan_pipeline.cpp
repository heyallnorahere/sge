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
#include "sge/platform/vulkan/vulkan_pipeline.h"
#include "sge/platform/vulkan/vulkan_context.h"
#include "sge/platform/vulkan/vulkan_shader.h"
#include "sge/platform/vulkan/vulkan_render_pass.h"
#include "sge/core/application.h"
#include "sge/renderer/renderer.h"
namespace sge {
    vulkan_pipeline::vulkan_pipeline(const pipeline_spec& spec) {
        this->m_spec = spec;

        {
            static const std::vector<VkDescriptorPoolSize> pool_sizes = {
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 },
                { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 }
            };

            auto create_info =
                vk_init<VkDescriptorPoolCreateInfo>(VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO);
            create_info.maxSets = 10;
            create_info.poolSizeCount = pool_sizes.size();
            create_info.pPoolSizes = pool_sizes.data();
            create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

            VkDevice device = vulkan_context::get().get_device().get();
            VkResult result = vkCreateDescriptorPool(device, &create_info, nullptr,
                                                     &this->m_descriptor_sets.pool);
            check_vk_result(result);
        }

        if (!this->m_spec._shader) {
            throw std::runtime_error("no shader was provided!");
        }
        renderer::add_shader_dependency(this->m_spec._shader, this);

        this->create();

        {
            auto vk_shader = this->m_spec._shader.as<vulkan_shader>();
            const auto& reflection_data = vk_shader->get_reflection_data();

            for (const auto& [name, resource] : reflection_data.resources) {
                if (resource.set > 0) {
                    continue;
                }

                using resource_type = vulkan_shader::resource_type;
                if (resource.type == resource_type::image ||
                    resource.type == resource_type::sampled_image) {
                    this->m_bindings[resource.binding].textures.resize(resource.descriptor_count);

                    ref<texture_2d> black_texture = renderer::get_black_texture();
                    for (uint32_t i = 0; i < resource.descriptor_count; i++) {
                        this->set_texture(black_texture, resource.binding, i);
                    }
                }
            }
        }
    }

    vulkan_pipeline::~vulkan_pipeline() {
        renderer::remove_shader_dependency(this->m_spec._shader, this);
        this->destroy();

        VkDevice device = vulkan_context::get().get_device().get();
        vkDestroyDescriptorPool(device, this->m_descriptor_sets.pool, nullptr);
    }

    void vulkan_pipeline::invalidate() {
        this->destroy();
        this->create();

        {
            std::vector<VkWriteDescriptorSet> writes;
            for (const auto& [binding, data] : this->m_bindings) {
                if (data.ubo) {
                    this->write(data.ubo, binding, writes);
                }

                for (size_t i = 0; i < data.textures.size(); i++) {
                    this->write(data.textures[i], binding, i, writes);
                }
            }

            if (!writes.empty()) {
                VkDevice device = vulkan_context::get().get_device().get();
                vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
            }
        }
    }

    void vulkan_pipeline::set_uniform_buffer(ref<uniform_buffer> ubo, uint32_t binding) {
        auto vk_uniform_buffer = ubo.as<vulkan_uniform_buffer>();

        {
            bool invalid_bind = false;
            if (this->m_bindings.find(binding) != this->m_bindings.end()) {
                if (!this->m_bindings[binding].textures.empty()) {
                    invalid_bind = true;
                }
            } else {
                this->m_bindings.insert(std::make_pair(binding, descriptor_set_binding_t()));
            }

            if (invalid_bind) {
                throw std::runtime_error("cannot bind a uniform buffer to binding " +
                                         std::to_string(binding) + "!");
            }

            this->m_bindings[binding].ubo = vk_uniform_buffer;
        }

        {
            std::vector<VkWriteDescriptorSet> writes;
            this->write(vk_uniform_buffer, binding, writes);

            VkDevice device = vulkan_context::get().get_device().get();
            vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
        }
    }

    void vulkan_pipeline::set_texture(ref<texture_2d> tex, uint32_t binding, uint32_t slot) {
        auto vk_texture = tex.as<vulkan_texture_2d>();

        {
            bool invalid_bind = false;
            if (this->m_bindings.find(binding) != this->m_bindings.end()) {
                if (this->m_bindings[binding].ubo) {
                    invalid_bind = true;
                }
            } else {
                this->m_bindings.insert(std::make_pair(binding, descriptor_set_binding_t()));
            }

            if (invalid_bind) {
                throw std::runtime_error("cannot bind a texture to binding " +
                                         std::to_string(binding) + "!");
            }

            auto& binding_data = this->m_bindings[binding];

            if (slot >= binding_data.textures.size()) {
                throw std::runtime_error("invalid texture slot!");
            }
            binding_data.textures[slot] = vk_texture;
        }

        {
            std::vector<VkWriteDescriptorSet> writes;
            this->write(vk_texture, binding, slot, writes);

            VkDevice device = vulkan_context::get().get_device().get();
            vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
        }
    }

    void vulkan_pipeline::get_descriptor_sets(
        std::map<uint32_t, std::vector<VkDescriptorSet>>& sets) {
        sets.clear();

        for (const auto& [set, data] : this->m_descriptor_sets.sets) {
            sets.insert(std::make_pair(set, data.sets));
        }
    }

    void vulkan_pipeline::create() {
        this->create_descriptor_sets();
        this->create_pipeline();
    }

    void vulkan_pipeline::destroy() {
        VkDevice device = vulkan_context::get().get_device().get();

        vkDestroyPipeline(device, this->m_pipeline, nullptr);
        vkDestroyPipelineLayout(device, this->m_layout, nullptr);

        for (const auto& [index, set] : this->m_descriptor_sets.sets) {
            vkFreeDescriptorSets(device, this->m_descriptor_sets.pool, set.sets.size(),
                                 set.sets.data());
            vkDestroyDescriptorSetLayout(device, set.layout, nullptr);
        }
        this->m_descriptor_sets.sets.clear();
    }

    struct set_binding_data {
        std::map<uint32_t, size_t> index_map;
        std::vector<VkDescriptorSetLayoutBinding> bindings;
    };

    void vulkan_pipeline::create_descriptor_sets() {
        auto vk_shader = this->m_spec._shader.as<vulkan_shader>();
        const auto& reflection_data = vk_shader->get_reflection_data();

        std::map<uint32_t, set_binding_data> bindings;
        for (const auto& [name, resource] : reflection_data.resources) {
            if (bindings.find(resource.set) == bindings.end()) {
                bindings.insert(std::make_pair(resource.set, set_binding_data()));
            }
            auto& set_bindings = bindings[resource.set];

            std::optional<size_t> duplicate_binding;
            if (set_bindings.index_map.find(resource.binding) != set_bindings.index_map.end()) {
                duplicate_binding = set_bindings.index_map[resource.binding];
            }

            using resource_type = vulkan_shader::resource_type;
            bool combined_sampler = false;
            if (duplicate_binding.has_value()) {
                if (resource.type == resource_type::image ||
                    resource.type == resource_type::sampler) {
                    const auto& duplicate_data = set_bindings.bindings[duplicate_binding.value()];
                    if (duplicate_data.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
                        duplicate_data.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER) {
                        combined_sampler = true;
                    }
                }

                if (!combined_sampler) {
                    throw std::runtime_error("incompatible resource types!");
                }
            }

            auto binding = vk_init<VkDescriptorSetLayoutBinding>();
            binding.binding = resource.binding;
            binding.stageFlags = vulkan_shader::get_shader_stage_flags(resource.stage);

            binding.descriptorCount = resource.descriptor_count;
            if (duplicate_binding.has_value()) {
                const auto& duplicate_data = set_bindings.bindings[duplicate_binding.value()];
                if (binding.descriptorCount < duplicate_data.descriptorCount) {
                    binding.descriptorCount = duplicate_data.descriptorCount;
                }
            }

            switch (resource.type) {
            case resource_type::uniform_buffer:
                binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                break;
            case resource_type::storage_buffer:
                binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                break;
            case resource_type::sampled_image:
                binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                break;
            case resource_type::sampler:
                binding.descriptorType = combined_sampler
                                             ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                                             : VK_DESCRIPTOR_TYPE_SAMPLER;
                break;
            case resource_type::image:
                binding.descriptorType = combined_sampler
                                             ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                                             : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                break;
            default:
                throw std::runtime_error("invalid resource type!");
            }

            if (duplicate_binding.has_value()) {
                set_bindings.bindings[duplicate_binding.value()] = binding;
            } else {
                set_bindings.index_map.insert(
                    std::make_pair(resource.binding, set_bindings.bindings.size()));
                set_bindings.bindings.push_back(binding);
            }
        }

        VkDevice device = vulkan_context::get().get_device().get();
        swapchain& swap_chain = application::get().get_swapchain();
        size_t image_count = swap_chain.get_image_count();
        for (const auto& [set, set_bindings] : bindings) {
            descriptor_set_t set_data;

            auto layout_info = vk_init<VkDescriptorSetLayoutCreateInfo>(
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
            layout_info.bindingCount = set_bindings.bindings.size();
            layout_info.pBindings = set_bindings.bindings.data();
            VkResult result =
                vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &set_data.layout);
            check_vk_result(result);

            std::vector<VkDescriptorSetLayout> layouts(image_count, set_data.layout);
            set_data.sets.resize(image_count);

            auto alloc_info = vk_init<VkDescriptorSetAllocateInfo>(
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO);
            alloc_info.descriptorPool = this->m_descriptor_sets.pool;
            alloc_info.descriptorSetCount = image_count;
            alloc_info.pSetLayouts = layouts.data();
            result = vkAllocateDescriptorSets(device, &alloc_info, set_data.sets.data());
            check_vk_result(result);

            this->m_descriptor_sets.sets.insert(std::make_pair(set, set_data));
        }
    }

    void vulkan_pipeline::create_pipeline() {
        VkDevice device = vulkan_context::get().get_device().get();
        auto vk_shader = this->m_spec._shader.as<vulkan_shader>();

        if (!this->m_spec.renderpass) {
            throw std::runtime_error("no render pass was provided!");
        }
        auto vk_render_pass = this->m_spec.renderpass.as<vulkan_render_pass>();

        const auto& push_constant_range = vk_shader->get_reflection_data().push_constant_buffer;
        VkPushConstantRange range;
        range.offset = 0;
        range.size = push_constant_range.size;
        range.stageFlags = push_constant_range.stage;

        auto layout_info =
            vk_init<VkPipelineLayoutCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);

        if (range.size > 0) {
            layout_info.pushConstantRangeCount = 1;
            layout_info.pPushConstantRanges = &range;
        }

        std::vector<VkDescriptorSetLayout> set_layouts;
        for (const auto& [set, data] : this->m_descriptor_sets.sets) {
            for (size_t i = 0; i < (size_t)set - set_layouts.size(); i++) {
                set_layouts.push_back(nullptr);
            }
            set_layouts.push_back(data.layout);
        }
        if (!set_layouts.empty()) {
            layout_info.setLayoutCount = set_layouts.size();
            layout_info.pSetLayouts = set_layouts.data();
        }

        VkResult result = vkCreatePipelineLayout(device, &layout_info, nullptr, &this->m_layout);
        check_vk_result(result);

        auto pipeline_info =
            vk_init<VkGraphicsPipelineCreateInfo>(VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO);
        pipeline_info.layout = this->m_layout;
        pipeline_info.renderPass = vk_render_pass->get();

        auto input_assembly = vk_init<VkPipelineInputAssemblyStateCreateInfo>(
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO);
        input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        auto rasterizer = vk_init<VkPipelineRasterizationStateCreateInfo>(
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO);
        rasterizer.polygonMode =
            (this->m_spec.wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL);
        rasterizer.cullMode =
            (this->m_spec.enable_culling ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE);
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthClampEnable = false;
        rasterizer.rasterizerDiscardEnable = false;
        rasterizer.depthBiasEnable = false;
        rasterizer.lineWidth = 1.f;

        auto blend_attachment_state = vk_init<VkPipelineColorBlendAttachmentState>();
        blend_attachment_state.colorWriteMask = 0xf;
        blend_attachment_state.blendEnable = true;
        
        switch (this->m_spec.renderpass->get_parent_type()) {
        case render_pass_parent_type::swap_chain:
            blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
            blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
            blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            break;
        default:
            throw std::runtime_error("this shouldn't be reached");
        }

        auto color_blend_state = vk_init<VkPipelineColorBlendStateCreateInfo>(
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO);
        color_blend_state.attachmentCount = 1;
        color_blend_state.pAttachments = &blend_attachment_state;

        auto viewport_state = vk_init<VkPipelineViewportStateCreateInfo>(
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO);
        viewport_state.viewportCount = 1;
        viewport_state.scissorCount = 1;

        std::vector<VkDynamicState> dynamic_states = { VK_DYNAMIC_STATE_VIEWPORT,
                                                       VK_DYNAMIC_STATE_SCISSOR };
        if (this->m_spec.wireframe) {
            dynamic_states.push_back(VK_DYNAMIC_STATE_LINE_WIDTH);
        }

        auto dynamic_state = vk_init<VkPipelineDynamicStateCreateInfo>(
            VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO);
        dynamic_state.dynamicStateCount = dynamic_states.size();
        dynamic_state.pDynamicStates = dynamic_states.data();

        // we might want to use a depth-stencil attachment? maybe?

        auto multisampling = vk_init<VkPipelineMultisampleStateCreateInfo>(
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO);
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        const auto& input_layout = this->m_spec.input_layout;

        VkVertexInputBindingDescription input_binding;
        input_binding.binding = 0;
        input_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        input_binding.stride = input_layout.stride;

        std::vector<VkVertexInputAttributeDescription> attributes;
        for (uint32_t i = 0; i < input_layout.attributes.size(); i++) {
            const auto& attribute = input_layout.attributes[i];

            VkVertexInputAttributeDescription attr_desc;
            attr_desc.binding = 0;
            attr_desc.location = i;
            attr_desc.offset = attribute.offset;

            switch (attribute.type) {
            case vertex_attribute_type::float1:
                attr_desc.format = VK_FORMAT_R32_SFLOAT;
                break;
            case vertex_attribute_type::float2:
                attr_desc.format = VK_FORMAT_R32G32_SFLOAT;
                break;
            case vertex_attribute_type::float3:
                attr_desc.format = VK_FORMAT_R32G32B32_SFLOAT;
                break;
            case vertex_attribute_type::float4:
                attr_desc.format = VK_FORMAT_R32G32B32A32_SFLOAT;
                break;
            case vertex_attribute_type::int1:
                attr_desc.format = VK_FORMAT_R32_SINT;
                break;
            case vertex_attribute_type::int2:
                attr_desc.format = VK_FORMAT_R32G32_SINT;
                break;
            case vertex_attribute_type::int3:
                attr_desc.format = VK_FORMAT_R32G32B32_SINT;
                break;
            case vertex_attribute_type::int4:
                attr_desc.format = VK_FORMAT_R32G32B32A32_SINT;
                break;
            case vertex_attribute_type::uint1:
                attr_desc.format = VK_FORMAT_R32_UINT;
                break;
            case vertex_attribute_type::uint2:
                attr_desc.format = VK_FORMAT_R32G32_UINT;
                break;
            case vertex_attribute_type::uint3:
                attr_desc.format = VK_FORMAT_R32G32B32_UINT;
                break;
            case vertex_attribute_type::uint4:
                attr_desc.format = VK_FORMAT_R32G32B32A32_UINT;
                break;
            case vertex_attribute_type::bool1:
                attr_desc.format = VK_FORMAT_R8_UINT;
                break;
            default:
                throw std::runtime_error("invalid attribute type!");
            }

            attributes.push_back(attr_desc);
        }

        auto input_state = vk_init<VkPipelineVertexInputStateCreateInfo>(
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO);
        input_state.vertexBindingDescriptionCount = 1;
        input_state.pVertexBindingDescriptions = &input_binding;

        if (!attributes.empty()) {
            input_state.vertexAttributeDescriptionCount = attributes.size();
            input_state.pVertexAttributeDescriptions = attributes.data();
        }

        const auto& stage_data = vk_shader->get_pipeline_info();
        if (!stage_data.empty()) {
            pipeline_info.stageCount = stage_data.size();
            pipeline_info.pStages = stage_data.data();
        }

        pipeline_info.pVertexInputState = &input_state;
        pipeline_info.pInputAssemblyState = &input_assembly;
        pipeline_info.pRasterizationState = &rasterizer;
        pipeline_info.pColorBlendState = &color_blend_state;
        pipeline_info.pMultisampleState = &multisampling;
        pipeline_info.pViewportState = &viewport_state;
        pipeline_info.pDepthStencilState = nullptr;
        pipeline_info.pDynamicState = &dynamic_state;

        result = vkCreateGraphicsPipelines(device, nullptr, 1, &pipeline_info, nullptr,
                                           &this->m_pipeline);
        check_vk_result(result);
    }

    // we're gonna have to assume descriptor set 0
    static constexpr uint32_t written_set = 0;

    void vulkan_pipeline::write(ref<vulkan_uniform_buffer> ubo, uint32_t binding,
                                std::vector<VkWriteDescriptorSet>& writes) {
        if (this->m_descriptor_sets.sets.find(written_set) == this->m_descriptor_sets.sets.end()) {
            throw std::runtime_error("descriptor set " + std::to_string(written_set) +
                                     " does not exist!");
        }

        for (VkDescriptorSet desc_set : this->m_descriptor_sets.sets[written_set].sets) {
            auto write = vk_init<VkWriteDescriptorSet>(VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);

            write.pBufferInfo = &ubo->get_descriptor_info();
            write.dstSet = desc_set;
            write.dstBinding = binding;
            write.dstArrayElement = 0;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

            writes.push_back(write);
        }
    }

    void vulkan_pipeline::write(ref<vulkan_texture_2d> tex, uint32_t binding, uint32_t slot, std::vector<VkWriteDescriptorSet>& writes) {
        if (this->m_descriptor_sets.sets.find(written_set) == this->m_descriptor_sets.sets.end()) {
            throw std::runtime_error("descriptor set " + std::to_string(written_set) +
                                     " does not exist!");
        }

        for (VkDescriptorSet desc_set : this->m_descriptor_sets.sets[written_set].sets) {
            auto write = vk_init<VkWriteDescriptorSet>(VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);

            write.pImageInfo = &tex->get_descriptor_info();
            write.dstSet = desc_set;
            write.dstBinding = binding;
            write.dstArrayElement = slot;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

            writes.push_back(write);
        }
    }
} // namespace sge