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
#include "sge/platform/vulkan/vulkan_command_list.h"
#include "sge/platform/vulkan/vulkan_context.h"
namespace sge {
    vulkan_command_list::vulkan_command_list(VkCommandPool command_pool) {
        m_command_pool = command_pool;

        auto alloc_info =
            vk_init<VkCommandBufferAllocateInfo>(VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandPool = m_command_pool;
        alloc_info.commandBufferCount = 1;

        VkDevice device = vulkan_context::get().get_device().get();
        VkResult result = vkAllocateCommandBuffers(device, &alloc_info, &m_command_buffer);
        check_vk_result(result);
    }

    vulkan_command_list::~vulkan_command_list() {
        VkDevice device = vulkan_context::get().get_device().get();
        vkFreeCommandBuffers(device, m_command_pool, 1, &m_command_buffer);
    }

    void vulkan_command_list::reset() { vkResetCommandBuffer(m_command_buffer, 0); }

    void vulkan_command_list::begin() {
        auto begin_info =
            vk_init<VkCommandBufferBeginInfo>(VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
        VkResult result = vkBeginCommandBuffer(m_command_buffer, &begin_info);
        check_vk_result(result);
    }

    void vulkan_command_list::end() {
        VkResult result = vkEndCommandBuffer(m_command_buffer);
        check_vk_result(result);
    }
} // namespace sge