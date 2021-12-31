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
#include "sge/platform/vulkan/vulkan_device.h"
#include "sge/platform/vulkan/vulkan_context.h"
namespace sge {
    std::vector<vulkan_physical_device> vulkan_physical_device::enumerate() {
        VkInstance instance = vulkan_context::get().get_instance();
        std::vector<vulkan_physical_device> devices;

        uint32_t physical_device_count = 0;
        vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);
        if (physical_device_count == 0) {
            return devices;
        }

        std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
        vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices.data());

        for (auto physical_device : physical_devices) {
            devices.push_back(vulkan_physical_device(physical_device));
        }
        return devices;
    }

    void vulkan_physical_device::query_queue_families(VkQueueFlags query, queue_family_indices& indices) const {
        uint32_t family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(this->m_device, &family_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families(family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(this->m_device,
            &family_count, queue_families.data());

        for (uint32_t i = 0; i < family_count; i++) {
            VkQueueFlags found = 0;
            const auto& queue_family = queue_families[i];

            auto query_family = [&](std::optional<uint32_t>& index, VkQueueFlagBits queue) {
                if (query & queue) {
                    if ((!index || queue_families[*index].queueFlags & queue == 0)
                        && (queue_family.queueFlags & queue)) {
                        index = i;
                    }

                    if (index) {
                        found |= queue;
                    }
                }
            };

            query_family(indices.graphics, VK_QUEUE_GRAPHICS_BIT);
            query_family(indices.compute, VK_QUEUE_COMPUTE_BIT);
            query_family(indices.transfer, VK_QUEUE_TRANSFER_BIT);

            if (found == query) {
                break;
            }
        }
    }

    void vulkan_physical_device::get_properties(VkPhysicalDeviceProperties& properties) const {
        vkGetPhysicalDeviceProperties(this->m_device, &properties);
    }

    void vulkan_physical_device::get_features(VkPhysicalDeviceFeatures& features) const {
        vkGetPhysicalDeviceFeatures(this->m_device, &features);
    }
}