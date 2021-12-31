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
#include "sge/platform/vulkan/vulkan_device.h"
namespace sge {
    struct vk_data;
    class vulkan_context {
    public:
        static void create(uint32_t version);
        static void destroy();
        static vulkan_context& get();

        vulkan_context(const vulkan_context&) = delete;
        vulkan_context& operator=(const vulkan_context&) = delete;

        uint32_t get_vulkan_version();
        VkInstance get_instance();
        vulkan_device& get_device();
        
        const std::set<std::string>& get_instance_extensions();
        const std::set<std::string>& get_device_extensions();
        const std::set<std::string>& get_layers();

    private:
        vulkan_context() = default;

        void init(uint32_t version);
        void shutdown();

        vk_data* m_data;
    };
}