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
#include "sge/platform/vulkan/vulkan_index_buffer.h"
namespace sge {
    vulkan_index_buffer::vulkan_index_buffer(const uint32_t* data, size_t count) {
        this->m_count = count;
        size_t size = this->m_count * sizeof(uint32_t);

        auto staging_buffer = ref<vulkan_buffer>::create(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU);
        staging_buffer->map();
        memcpy(staging_buffer->mapped, data, size);
        staging_buffer->unmap();

        VkBufferCopy region;
        region.size = size;
        region.srcOffset = region.dstOffset = 0;

        this->m_buffer = ref<vulkan_buffer>::create(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
        staging_buffer->copy_to(this->m_buffer, region);
    }
}