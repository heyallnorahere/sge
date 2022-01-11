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
#include "sge/platform/vulkan/vulkan_vertex_buffer.h"
namespace sge {
    vulkan_vertex_buffer::vulkan_vertex_buffer(const void* data, size_t stride, size_t count) {
        m_stride = stride;
        m_count = count;
        size_t size = m_stride * m_count;

        auto staging_buffer = ref<vulkan_buffer>::create(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                         VMA_MEMORY_USAGE_CPU_TO_GPU);
        staging_buffer->map();
        memcpy(staging_buffer->mapped, data, size);
        staging_buffer->unmap();

        VkBufferCopy region;
        region.size = size;
        region.srcOffset = region.dstOffset = 0;

        m_buffer = ref<vulkan_buffer>::create(
            size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);
        staging_buffer->copy_to(m_buffer, region);
    }
} // namespace sge