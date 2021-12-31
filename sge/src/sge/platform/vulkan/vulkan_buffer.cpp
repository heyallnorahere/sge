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

#include "sgepch.h"
#include "sge/platform/vulkan/vulkan_base.h"
#include "sge/platform/vulkan/vulkan_buffer.h"
#include "sge/platform/vulkan/vulkan_allocator.h"
#include "sge/platform/vulkan/vulkan_context.h"
#include "sge/platform/vulkan/vulkan_command_list.h"
#include "sge/renderer/renderer.h"
namespace sge {
    vulkan_buffer::vulkan_buffer(size_t size, VkBufferUsageFlags buffer_usage,
        VmaMemoryUsage memory_usage) {
        this->m_size = size;
        this->m_buffer_usage = buffer_usage;
        this->m_memory_usage = memory_usage;

        this->create();
    }

    vulkan_buffer::~vulkan_buffer() {
        vulkan_allocator::free(this->m_buffer, this->m_allocation);
    }

    void vulkan_buffer::map() {
        if (this->mapped != nullptr) {
            return;
        }

        this->mapped = vulkan_allocator::map(this->m_allocation);
    }

    void vulkan_buffer::unmap() {
        if (this->mapped == nullptr) {
            return;
        }

        vulkan_allocator::unmap(this->m_allocation);
        this->mapped = nullptr;
    }

    void vulkan_buffer::copy_to(ref<vulkan_buffer> dest, const VkBufferCopy& region) {
        auto queue = renderer::get_queue(command_list_type::transfer);
        auto& cmdlist = (vulkan_command_list&)queue->get();
        cmdlist.begin();

        VkCommandBuffer cmdbuffer = cmdlist.get();
        vkCmdCopyBuffer(cmdbuffer, this->m_buffer, dest->m_buffer, 1, &region);

        cmdlist.end();
        queue->submit(cmdlist, true);
    }

    void vulkan_buffer::create() {
        auto create_info = vk_init<VkBufferCreateInfo>(VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
        create_info.size = this->m_size;
        create_info.usage = this->m_buffer_usage;
        
        auto physical_device = vulkan_context::get().get_device().get_physical_device();
        vulkan_physical_device::queue_family_indices indices;
        VkQueueFlags query = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
        physical_device.query_queue_families(query, indices);

        std::set<uint32_t> index_set = {
            indices.graphics.value(),
            indices.compute.value(),
            indices.transfer.value()
        };
        std::vector<uint32_t> queue_families(index_set.begin(), index_set.end());

        if (queue_families.size() > 1) {
            create_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = queue_families.size();
            create_info.pQueueFamilyIndices = queue_families.data();
        } else {
            create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        auto alloc_info = vk_init<VmaAllocationCreateInfo>();
        alloc_info.usage = this->m_memory_usage;

        vulkan_allocator::alloc(create_info, alloc_info, this->m_buffer, this->m_allocation);
    }
}