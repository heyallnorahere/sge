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
#include "sge/platform/vulkan/vulkan_renderer.h"
#include "sge/platform/vulkan/vulkan_base.h"
#include "sge/core/application.h"
namespace sge {
    struct vk_renderer_data {
        uint32_t vulkan_version = 0;
        std::set<std::string> instance_extensions, device_extensions, layer_names;
        VkInstance instance = nullptr;
    };

    static void choose_extensions(vk_renderer_data* data) {
        ref<application> app = application::get();
        app->get_window().get_vulkan_extensions(data->instance_extensions);

        data->instance_extensions.insert("VK_KHR_get_physical_device_properties2");
        data->device_extensions.insert("VK_KHR_swapchain");

#ifdef SGE_DEBUG
        data->instance_extensions.insert("VK_EXT_debug_utils");
        data->layer_names.insert("VK_LAYER_KHRONOS_validation");
#endif
    }

    static void create_instance(vk_renderer_data* data) {
        auto app_info = vk_init<VkApplicationInfo>(VK_STRUCTURE_TYPE_APPLICATION_INFO);
        app_info.apiVersion = data->vulkan_version;

        app_info.pEngineName = "sge";
        app_info.engineVersion = VK_MAKE_VERSION(0, 0, 1); // todo: cmake version

        ref<application> app = application::get();
        app_info.pApplicationName = app->get_title().c_str();
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);

        auto create_info = vk_init<VkInstanceCreateInfo>(VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);
        create_info.pApplicationInfo = &app_info;

        std::vector<const char*> extensions;
        for (const auto& extension_name : data->instance_extensions) {
            extensions.push_back(extension_name.c_str());
        }
        create_info.ppEnabledExtensionNames = extensions.data();
        create_info.enabledExtensionCount = extensions.size();

        std::vector<const char*> layers;
        for (const auto& layer_name : data->layer_names) {
            layers.push_back(layer_name.c_str());
        }
        create_info.ppEnabledLayerNames = layers.data();
        create_info.enabledLayerCount = layers.size();

        VkResult result = vkCreateInstance(&create_info, nullptr, &data->instance);
        check_vk_result(result);
    }

    void vulkan_renderer::init() {
        this->m_data = new vk_renderer_data;
        this->m_data->vulkan_version = VK_API_VERSION_1_1;

        choose_extensions(this->m_data);
        create_instance(this->m_data);
    }

    void vulkan_renderer::shutdown() {
        vkDestroyInstance(this->m_data->instance, nullptr);

        delete this->m_data;
    }
}