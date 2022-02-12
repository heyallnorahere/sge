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

#pragma once
namespace sge {
    class vulkan_physical_device {
    public:
        struct queue_family_indices {
            std::optional<uint32_t> graphics, compute, transfer;

            std::map<VkQueueFlagBits, uint32_t> map() {
                std::map<VkQueueFlagBits, uint32_t> result;

                if (graphics.has_value()) {
                    result.insert(std::make_pair(VK_QUEUE_GRAPHICS_BIT, graphics.value()));
                }
                if (compute.has_value()) {
                    result.insert(std::make_pair(VK_QUEUE_COMPUTE_BIT, compute.value()));
                }
                if (transfer.has_value()) {
                    result.insert(std::make_pair(VK_QUEUE_TRANSFER_BIT, transfer.value()));
                }

                return result;
            }
        };

        static std::vector<vulkan_physical_device> enumerate();

        vulkan_physical_device() { m_device = nullptr; }

        vulkan_physical_device(const vulkan_physical_device&) = default;
        vulkan_physical_device& operator=(const vulkan_physical_device&) = default;

        bool is_extension_supported(const std::string& name) const;
        void query_queue_families(VkQueueFlags query, queue_family_indices& indices) const;
        void get_properties(VkPhysicalDeviceProperties& properties) const;
        void get_features(VkPhysicalDeviceFeatures& features) const;

        void get_surface_capabilities(VkSurfaceKHR surface,
                                      VkSurfaceCapabilitiesKHR& capabilities) const;
        void get_surface_formats(VkSurfaceKHR surface,
                                 std::vector<VkSurfaceFormatKHR>& formats) const;
        void get_surface_present_modes(VkSurfaceKHR surface,
                                       std::vector<VkPresentModeKHR>& present_modes) const;
        std::optional<uint32_t> find_surface_present_queue(VkSurfaceKHR surface) const;

        VkPhysicalDevice get() const { return m_device; }

        operator bool() const { return m_device != nullptr; }

        bool operator==(const vulkan_physical_device& other) const {
            return m_device == other.m_device;
        }
        bool operator!=(const vulkan_physical_device& other) const { return !(*this == other); }

    private:
        vulkan_physical_device(VkPhysicalDevice device) { m_device = device; }

        VkPhysicalDevice m_device;
    };

    class vulkan_device {
    public:
        vulkan_device(const vulkan_physical_device& physical_device);
        ~vulkan_device();

        vulkan_device(const vulkan_device&) = delete;
        vulkan_device& operator=(const vulkan_device&) = delete;

        VkDevice get() { return m_device; };
        vulkan_physical_device get_physical_device() { return m_physical_device; }
        VkQueue get_queue(uint32_t family);

    private:
        void create();

        VkDevice m_device;
        vulkan_physical_device m_physical_device;
        void* m_crash_tracker;
    };
} // namespace sge