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
#include "sge/platform/vulkan/vulkan_uniform_buffer.h"
namespace sge {
    vulkan_uniform_buffer::vulkan_uniform_buffer(size_t size) {
        this->m_buffer = ref<vulkan_buffer>::create(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                    VMA_MEMORY_USAGE_CPU_ONLY);
    }

    void vulkan_uniform_buffer::set_data(const void* data, size_t size, size_t offset) {
        size_t buffer_size = this->m_buffer->size();
        if (offset + size > buffer_size) {
            throw std::runtime_error("cannot copy to outside buffer memory!");
        }

        this->m_buffer->map();
        void* dest = (void*)((size_t)this->m_buffer->mapped + offset);
        memcpy(dest, data, size);
        this->m_buffer->unmap();
    }
} // namespace sge