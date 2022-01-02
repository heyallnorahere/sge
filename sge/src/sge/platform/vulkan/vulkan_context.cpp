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
#include "sge/platform/vulkan/vulkan_context.h"
#include "sge/core/application.h"
#include "sge/platform/vulkan/vulkan_allocator.h"
namespace sge {
    static std::unique_ptr<vulkan_context> vk_context_instance;

    void vulkan_context::create(uint32_t version) {
        if (vk_context_instance) {
            return;
        }

        auto instance = new vulkan_context;
        vk_context_instance = std::unique_ptr<vulkan_context>(instance);
        vk_context_instance->init(version);
    }

    void vulkan_context::destroy() {
        if (!vk_context_instance) {
            return;
        }

        vk_context_instance->shutdown();
        vk_context_instance.reset();
    }

    vulkan_context& vulkan_context::get() { return *vk_context_instance; }

    struct vk_data {
        uint32_t vulkan_version;
        std::set<std::string> instance_extensions, device_extensions, layer_names;
        VkInstance instance = nullptr;
        VkDebugUtilsMessengerEXT debug_messenger = nullptr;
        std::unique_ptr<vulkan_device> device;
    };

    static void choose_extensions(vk_data* data) {
        auto& app = application::get();
        app.get_window()->get_vulkan_extensions(data->instance_extensions);

        data->instance_extensions.insert("VK_KHR_get_physical_device_properties2");
        data->device_extensions.insert("VK_KHR_swapchain");

#ifdef SGE_DEBUG
        data->instance_extensions.insert("VK_EXT_debug_utils");
        data->layer_names.insert("VK_LAYER_KHRONOS_validation");
#endif

        if (data->vulkan_version < VK_API_VERSION_1_1) {
            data->device_extensions.insert("VK_KHR_maintenance1");
        }
    }

    static void create_instance(vk_data* data) {
        auto app_info = vk_init<VkApplicationInfo>(VK_STRUCTURE_TYPE_APPLICATION_INFO);
        app_info.apiVersion = data->vulkan_version;

        app_info.pEngineName = "sge";
        app_info.engineVersion = VK_MAKE_VERSION(0, 0, 1); // todo: cmake version

        auto& app = application::get();
        app_info.pApplicationName = app.get_title().c_str();
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);

        auto create_info = vk_init<VkInstanceCreateInfo>(VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);
        create_info.pApplicationInfo = &app_info;

        std::vector<const char*> instance_extensions;
        uint32_t extension_count = 0;
        {
            vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
            std::vector<VkExtensionProperties> extensions(extension_count);
            vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

            // verify extension availability
            for (const auto& selected_extension : data->instance_extensions) {
                bool found = false;
                for (const auto& available_extension : extensions) {
                    if (selected_extension == available_extension.extensionName) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    instance_extensions.push_back(selected_extension.c_str());
                } else {
                    spdlog::warn("instance extension " + selected_extension + " is not present");
                }
            }
        }

        std::vector<const char*> instance_layers;
        uint32_t layer_count = 0;
        {
            vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
            std::vector<VkLayerProperties> layers(layer_count);
            vkEnumerateInstanceLayerProperties(&layer_count, layers.data());

            // verify layer availability
            for (const auto& selected_layer : data->layer_names) {
                bool found = false;
                for (const auto& available_layer : layers) {
                    if (selected_layer == available_layer.layerName) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    instance_layers.push_back(selected_layer.c_str());
                } else {
                    spdlog::warn("instance layer " + selected_layer + " is not present");
                }
            }
        }

        if (!instance_extensions.empty()) {
            create_info.ppEnabledExtensionNames = instance_extensions.data();
            create_info.enabledExtensionCount = instance_extensions.size();
        }

        if (!instance_layers.empty()) {
            create_info.ppEnabledLayerNames = instance_layers.data();
            create_info.enabledLayerCount = instance_layers.size();
        }

        {
            uint32_t major = VK_VERSION_MAJOR(data->vulkan_version);
            uint32_t minor = VK_VERSION_MINOR(data->vulkan_version);
            uint32_t patch = VK_VERSION_PATCH(data->vulkan_version);

            spdlog::info("creating vulkan instance:\n\tusing api version: {0}.{1}.{2}\n\tavailable "
                         "instance extensions: {3}\n\tavailable instance layers: {4}",
                         major, minor, patch, extension_count, layer_count);
        }

        VkResult result = vkCreateInstance(&create_info, nullptr, &data->instance);
        check_vk_result(result);
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_validation_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT message_type,
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data) {
        std::string message = "validation layer: " + std::string(callback_data->pMessage);

        switch (severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            spdlog::warn(message);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            spdlog::error(message);
            break;
        default:
            // not important enough to show
            break;
        }

        return false;
    }

    static void create_debug_messenger(vk_data* data) {
        auto fpCreateDebugUtilsMessengerEXT =
            (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                data->instance, "vkCreateDebugUtilsMessengerEXT");
        if (fpCreateDebugUtilsMessengerEXT == nullptr) {
            return;
        }

        auto create_info = vk_init<VkDebugUtilsMessengerCreateInfoEXT>(
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT);

        create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

        create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

        create_info.pfnUserCallback = vulkan_validation_callback;
        create_info.pUserData = nullptr;

        VkResult result = fpCreateDebugUtilsMessengerEXT(data->instance, &create_info, nullptr,
                                                         &data->debug_messenger);
        check_vk_result(result);
    }

    static vulkan_physical_device choose_physical_device() {
        auto candidates = vulkan_physical_device::enumerate();
        if (candidates.empty()) {
            throw std::runtime_error("no vulkan-supporting device was found!");
        }
        vulkan_physical_device selected;

        for (const auto& candidate : candidates) {
            VkPhysicalDeviceProperties properties;
            candidate.get_properties(properties);
            // good enough
            if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                selected = candidate;
                break;
            }
        }

        if (!selected) {
            selected = candidates[0];
        }
        return selected;
    }

    void vulkan_context::init(uint32_t version) {
        this->m_data = new vk_data;
        this->m_data->vulkan_version = version;

        choose_extensions(this->m_data);
        create_instance(this->m_data);
        create_debug_messenger(this->m_data);

        {
            auto physical_device = choose_physical_device();

            VkPhysicalDeviceProperties properties;
            physical_device.get_properties(properties);

            uint32_t major = VK_VERSION_MAJOR(properties.apiVersion);
            uint32_t minor = VK_VERSION_MINOR(properties.apiVersion);
            uint32_t patch = VK_VERSION_PATCH(properties.apiVersion);

            uint32_t extension_count = 0;
            uint32_t layer_count = 0;
            {
                VkPhysicalDevice vk_device = physical_device.get();

                vkEnumerateDeviceExtensionProperties(vk_device, nullptr, &extension_count, nullptr);
                vkEnumerateDeviceLayerProperties(vk_device, &layer_count, nullptr);
            }

            spdlog::info("selected physical device:\n\tname: {0}"
                         "\n\tlatest available vulkan version: {1}.{2}.{3}\n\tavailable device "
                         "extensions: {4}\n\tavailable device layers: {5}",
                         properties.deviceName, major, minor, patch, extension_count, layer_count);

            this->m_data->device = std::make_unique<vulkan_device>(physical_device);
        }

        vulkan_allocator::init();
    }

    void vulkan_context::shutdown() {
        vulkan_allocator::shutdown();

        this->m_data->device.reset();

        if (this->m_data->debug_messenger != nullptr) {
            auto fpDestroyDebugUtilsMessengerEXT =
                (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                    this->m_data->instance, "vkDestroyDebugUtilsMessengerEXT");

            if (fpDestroyDebugUtilsMessengerEXT != nullptr) {
                fpDestroyDebugUtilsMessengerEXT(this->m_data->instance,
                                                this->m_data->debug_messenger, nullptr);
                this->m_data->debug_messenger = nullptr;
            } else {
                spdlog::warn("created debug messenger but could not destroy it - "
                             "will result in memory leak");
            }
        }

        vkDestroyInstance(this->m_data->instance, nullptr);
        this->m_data->instance = nullptr;

        delete this->m_data;
    }

    uint32_t vulkan_context::get_vulkan_version() { return this->m_data->vulkan_version; }
    VkInstance vulkan_context::get_instance() { return this->m_data->instance; }
    vulkan_device& vulkan_context::get_device() { return *this->m_data->device; }

    const std::set<std::string>& vulkan_context::get_instance_extensions() {
        return this->m_data->instance_extensions;
    }
    const std::set<std::string>& vulkan_context::get_device_extensions() {
        return this->m_data->device_extensions;
    }
    const std::set<std::string>& vulkan_context::get_layers() { return this->m_data->layer_names; }
} // namespace sge