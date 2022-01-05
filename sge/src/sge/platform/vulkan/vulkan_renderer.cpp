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
#include "sge/platform/vulkan/vulkan_renderer.h"
#include "sge/platform/vulkan/vulkan_base.h"
#include "sge/platform/vulkan/vulkan_context.h"
#include "sge/platform/vulkan/vulkan_command_list.h"
#include "sge/platform/vulkan/vulkan_vertex_buffer.h"
#include "sge/platform/vulkan/vulkan_index_buffer.h"
#include "sge/platform/vulkan/vulkan_pipeline.h"
#include "sge/core/application.h"
namespace sge {
    void vulkan_renderer::init() { vulkan_context::create(VK_API_VERSION_1_1); }
    void vulkan_renderer::shutdown() { vulkan_context::destroy(); }

    void vulkan_renderer::wait() {
        VkDevice device = vulkan_context::get().get_device().get();
        vkDeviceWaitIdle(device);
    }

    void vulkan_renderer::submit(const draw_data& data) {
        auto vk_cmdlist = (vulkan_command_list*)data.cmdlist;
        VkCommandBuffer cmdbuffer = vk_cmdlist->get();

        // see vulkan_pipeline.cpp:406
        vkCmdSetLineWidth(cmdbuffer, 1.f);

        auto vk_vertex_buffer = data.vertices.as<vulkan_vertex_buffer>();
        VkBuffer vbo = vk_vertex_buffer->get()->get();
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmdbuffer, 0, 1, &vbo, &offset);

        auto vk_index_buffer = data.indices.as<vulkan_index_buffer>();
        VkBuffer ibo = vk_index_buffer->get()->get();
        vkCmdBindIndexBuffer(cmdbuffer, ibo, 0, VK_INDEX_TYPE_UINT32);

        auto vk_pipeline = data._pipeline.as<vulkan_pipeline>();
        vkCmdBindPipeline(cmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline->get_pipeline());

        swapchain& swap_chain = application::get().get_swapchain();
        size_t current_image = swap_chain.get_current_image_index();

        VkPipelineLayout pipeline_layout = vk_pipeline->get_pipeline_layout();
        std::map<uint32_t, std::vector<VkDescriptorSet>> sets;
        vk_pipeline->get_descriptor_sets(sets);
        for (const auto& [set, data] : sets) {
            vkCmdBindDescriptorSets(cmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout,
                                    set, 1, &data[current_image], 0, nullptr);
        }

        vkCmdDrawIndexed(cmdbuffer, data.indices->get_index_count(), 1, 0, 0, 0);
    }
} // namespace sge