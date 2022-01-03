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
#include "sge/platform/vulkan/vulkan_allocator.h"
#include "sge/platform/vulkan/vulkan_context.h"
namespace sge {
    static VmaAllocator vk_allocator = nullptr;

    void vulkan_allocator::init() {
        if (vk_allocator != nullptr) {
            return;
        }

        vulkan_context& context = vulkan_context::get();
        vulkan_device& device = context.get_device();

        auto create_info = vk_init<VmaAllocatorCreateInfo>();
        create_info.instance = context.get_instance();
        create_info.physicalDevice = device.get_physical_device().get();
        create_info.device = device.get();
        create_info.vulkanApiVersion = context.get_vulkan_version();

#if VMA_DYNAMIC_VULKAN_FUNCTIONS
        auto functions = vk_init<VmaVulkanFunctions>();
        functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
        create_info.pVulkanFunctions = &functions;
#endif

        VkResult result = vmaCreateAllocator(&create_info, &vk_allocator);
        check_vk_result(result);
    }

    void vulkan_allocator::shutdown() {
        if (vk_allocator == nullptr) {
            return;
        }

        vmaDestroyAllocator(vk_allocator);
        vk_allocator = nullptr;
    }

    void vulkan_allocator::alloc(const VkBufferCreateInfo& create_info,
                                 const VmaAllocationCreateInfo& alloc_info, VkBuffer& buffer,
                                 VmaAllocation& allocation) {
        VkResult result =
            vmaCreateBuffer(vk_allocator, &create_info, &alloc_info, &buffer, &allocation, nullptr);
        check_vk_result(result);
    }

    void vulkan_allocator::free(VkBuffer buffer, VmaAllocation allocation) {
        vmaDestroyBuffer(vk_allocator, buffer, allocation);
    }

    void vulkan_allocator::alloc(const VkImageCreateInfo& create_info,
                                 const VmaAllocationCreateInfo& alloc_info, VkImage& image,
                                 VmaAllocation& allocation) {
        VkResult result =
            vmaCreateImage(vk_allocator, &create_info, &alloc_info, &image, &allocation, nullptr);
        check_vk_result(result);
    }

    void vulkan_allocator::free(VkImage image, VmaAllocation allocation) {
        vmaDestroyImage(vk_allocator, image, allocation);
    }

    void* vulkan_allocator::map(VmaAllocation allocation) {
        void* gpu_data;
        VkResult result = vmaMapMemory(vk_allocator, allocation, &gpu_data);
        check_vk_result(result);
        return gpu_data;
    }

    void vulkan_allocator::unmap(VmaAllocation allocation) {
        vmaUnmapMemory(vk_allocator, allocation);
    }
} // namespace sge