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
    class vulkan_physical_device {
    public:
        struct queue_family_indices {
            std::optional<uint32_t> graphics, compute, transfer;
        };

        static std::vector<vulkan_physical_device> enumerate();

        vulkan_physical_device() { this->m_device = nullptr; }

        vulkan_physical_device(const vulkan_physical_device&) = default;
        vulkan_physical_device& operator=(const vulkan_physical_device&) = default;

        void query_queue_families(VkQueueFlags query, queue_family_indices& indices) const;
        void get_properties(VkPhysicalDeviceProperties& properties) const;
        void get_features(VkPhysicalDeviceFeatures& features) const;

        VkPhysicalDevice get() const { return this->m_device; }

        operator bool() const { return this->m_device != nullptr; }
        operator VkPhysicalDevice() const { return this->m_device; }

        bool operator==(const vulkan_physical_device& other) const {
            return this->m_device == other.m_device;
        }
        bool operator!=(const vulkan_physical_device& other) const {
            return !(*this == other);
        }

    private:
        vulkan_physical_device(VkPhysicalDevice device) { this->m_device = device; }

        VkPhysicalDevice m_device;
    };
}