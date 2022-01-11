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
#include "sge/platform/vulkan/vulkan_command_queue.h"
#include "sge/platform/vulkan/vulkan_context.h"
#include "sge/platform/vulkan/vulkan_command_list.h"
namespace sge {
    vulkan_command_queue::vulkan_command_queue(command_list_type type) {
        m_type = type;

        VkQueueFlagBits query;
        switch (m_type) {
        case command_list_type::graphics:
            query = VK_QUEUE_GRAPHICS_BIT;
            break;
        case command_list_type::compute:
            query = VK_QUEUE_COMPUTE_BIT;
            break;
        case command_list_type::transfer:
            query = VK_QUEUE_TRANSFER_BIT;
            break;
        default:
            throw std::runtime_error("invalid command list type!");
        };

        vulkan_device& device = vulkan_context::get().get_device();
        auto physical_device = device.get_physical_device();

        vulkan_physical_device::queue_family_indices indices;
        physical_device.query_queue_families(query, indices);
        auto map = indices.map();
        uint32_t queue_family = map[query];

        m_queue = device.get_queue(queue_family);

        {
            auto create_info =
                vk_init<VkCommandPoolCreateInfo>(VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
            create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

            VkResult result =
                vkCreateCommandPool(device.get(), &create_info, nullptr, &m_command_pool);
            check_vk_result(result);
        }
    }

    vulkan_command_queue::~vulkan_command_queue() {
        vkQueueWaitIdle(m_queue);

        VkDevice device = vulkan_context::get().get_device().get();
        while (!m_command_lists.empty()) {
            auto& front = m_command_lists.front();
            vkDestroyFence(device, front.fence, nullptr);
            front.cmdlist.reset();

            m_command_lists.pop();
        }

        vkDestroyCommandPool(device, m_command_pool, nullptr);
    }

    static bool is_fence_complete(VkDevice device, VkFence fence) {
        VkResult status = vkGetFenceStatus(device, fence);
        return status == VK_SUCCESS;
    }

    command_list& vulkan_command_queue::get() {
        VkDevice device = vulkan_context::get().get_device().get();

        command_list* cmdlist;
        if (!m_command_lists.empty() &&
            is_fence_complete(device, m_command_lists.front().fence)) {
            auto& front = m_command_lists.front();
            cmdlist = front.cmdlist.release();
            cmdlist->reset();
            vkDestroyFence(device, front.fence, nullptr);

            m_command_lists.pop();
        } else {
            cmdlist = new vulkan_command_list(m_command_pool);
        }

        return *cmdlist;
    }

    void vulkan_command_queue::submit(command_list& cmdlist, bool wait) {
        VkDevice device = vulkan_context::get().get_device().get();
        auto vk_cmdlist = (vulkan_command_list*)&cmdlist;
        VkCommandBuffer cmdbuffer = vk_cmdlist->get();

        auto submit_info = vk_init<VkSubmitInfo>(VK_STRUCTURE_TYPE_SUBMIT_INFO);
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &cmdbuffer;

        auto fence_info = vk_init<VkFenceCreateInfo>(VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
        VkFence fence;
        VkResult result = vkCreateFence(device, &fence_info, nullptr, &fence);
        check_vk_result(result);

        result = vkQueueSubmit(m_queue, 1, &submit_info, fence);
        check_vk_result(result);

        if (wait) {
            vkWaitForFences(device, 1, &fence, true, std::numeric_limits<uint64_t>::max());
        }

        auto unique_ptr = std::unique_ptr<command_list>(&cmdlist);
        m_command_lists.push({ std::move(unique_ptr), fence });
    }
} // namespace sge