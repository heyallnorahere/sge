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

    void vulkan_physical_device::query_queue_families(VkQueueFlags query,
                                                      queue_family_indices& indices) const {
        uint32_t family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(this->m_device, &family_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families(family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(this->m_device, &family_count,
                                                 queue_families.data());

        for (uint32_t i = 0; i < family_count; i++) {
            VkQueueFlags found = 0;
            const auto& queue_family = queue_families[i];

            auto query_family = [&](std::optional<uint32_t>& index, VkQueueFlagBits queue) {
                if (query & queue) {
                    if ((!index || (queue_families[*index].queueFlags & queue) == 0) &&
                        (queue_family.queueFlags & queue)) {
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

    vulkan_device::vulkan_device(const vulkan_physical_device& physical_device) {
        this->m_physical_device = physical_device;
        this->create();
    }

    vulkan_device::~vulkan_device() {
        vkDeviceWaitIdle(this->m_device);
        vkDestroyDevice(this->m_device, nullptr);
    }

    VkQueue vulkan_device::get_queue(uint32_t family) {
        VkQueue queue;
        vkGetDeviceQueue(this->m_device, family, 0, &queue);
        return queue;
    }

    void vulkan_device::create() {
        auto& context = vulkan_context::get();
        VkPhysicalDevice physical_device = this->m_physical_device.get();

        const auto& selected_extensions = context.get_device_extensions();
        std::vector<const char*> device_extensions;
        {
            uint32_t extension_count = 0;
            vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count,
                                                 nullptr);
            std::vector<VkExtensionProperties> extensions(extension_count);
            vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count,
                                                 extensions.data());

            // verify extension availability
            for (const auto& selected_extension : selected_extensions) {
                bool found = false;
                for (const auto& extension : extensions) {
                    if (selected_extension == extension.extensionName) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    device_extensions.push_back(selected_extension.c_str());
                } else {
                    spdlog::warn("device extension {0} is not present", selected_extension);
                }
            }

            // if VK_KHR_portability_subset is available, add it
            // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VK_KHR_portability_subset.html
            static const char* const portability_subset = "VK_KHR_portability_subset";
            for (const auto& extension : extensions) {
                bool requested =
                    selected_extensions.find(extension.extensionName) == selected_extensions.end();

                if (!requested && (strcmp(portability_subset, extension.extensionName) == 0)) {
                    device_extensions.push_back(portability_subset);
                    break;
                }
            }
        }

        const auto& layer_names = context.get_layers();
        std::vector<const char*> device_layers;
        {
            uint32_t layer_count = 0;
            vkEnumerateDeviceLayerProperties(physical_device, &layer_count, nullptr);
            std::vector<VkLayerProperties> layers(layer_count);
            vkEnumerateDeviceLayerProperties(physical_device, &layer_count, layers.data());

            // verify layer availability
            for (const auto& selected_layer : layer_names) {
                bool found = false;
                for (const auto& layer_data : layers) {
                    if (selected_layer == layer_data.layerName) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    device_layers.push_back(selected_layer.c_str());
                } else {
                    spdlog::warn("device layer {0} is not present", selected_layer);
                }
            }
        }

        static const float queue_priority = 1.f;
        std::vector<VkDeviceQueueCreateInfo> queue_create_info;

        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
        for (uint32_t i = 0; i < queue_family_count; i++) {
            auto create_info =
                vk_init<VkDeviceQueueCreateInfo>(VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO);

            create_info.queueFamilyIndex = i;
            create_info.queueCount = 1;
            create_info.pQueuePriorities = &queue_priority;

            queue_create_info.push_back(create_info);
        }

        auto create_info = vk_init<VkDeviceCreateInfo>(VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO);
        create_info.pQueueCreateInfos = queue_create_info.data();
        create_info.queueCreateInfoCount = queue_create_info.size();

        VkPhysicalDeviceFeatures features;
        this->m_physical_device.get_features(features);
        create_info.pEnabledFeatures = &features;

        if (!device_extensions.empty()) {
            create_info.ppEnabledExtensionNames = device_extensions.data();
            create_info.enabledExtensionCount = device_extensions.size();
        }

        if (!device_layers.empty()) {
            create_info.ppEnabledLayerNames = device_layers.data();
            create_info.enabledLayerCount = device_layers.size();
        }

        VkResult result = vkCreateDevice(physical_device, &create_info, nullptr, &this->m_device);
        check_vk_result(result);
    }
} // namespace sge