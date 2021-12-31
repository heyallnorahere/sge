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
    class vulkan_allocator {
    public:
        static void init();
        static void shutdown();

        vulkan_allocator() = delete;

        // buffers
        static void alloc(const VkBufferCreateInfo& create_info,
                          const VmaAllocationCreateInfo& alloc_info, VkBuffer& buffer,
                          VmaAllocation& allocation);
        static void free(VkBuffer buffer, VmaAllocation allocation);

        // mapping memory
        static void* map(VmaAllocation allocation);
        static void unmap(VmaAllocation allocation);
    };
} // namespace sge