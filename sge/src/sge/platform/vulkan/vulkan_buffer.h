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
namespace sge {
    class vulkan_buffer : public ref_counted {
    public:
        vulkan_buffer(size_t size, VkBufferUsageFlags buffer_usage, VmaMemoryUsage memory_usage);
        ~vulkan_buffer();

        vulkan_buffer(const vulkan_buffer&) = delete;
        vulkan_buffer& operator=(const vulkan_buffer&) = delete;

        void* mapped = nullptr;
        void map();
        void unmap();

        void copy_to(ref<vulkan_buffer> dest, const VkBufferCopy& region);

        VkBuffer get() { return this->m_buffer; }
        size_t size() { return this->m_size; }
        VkBufferUsageFlags get_buffer_usage() { return this->m_buffer_usage; }
        VmaMemoryUsage get_memory_usage() { return this->m_memory_usage; }

    private:
        void create();

        VkBuffer m_buffer;
        VmaAllocation m_allocation;
        size_t m_size;

        VkBufferUsageFlags m_buffer_usage;
        VmaMemoryUsage m_memory_usage;
    };
} // namespace sge