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
#include "sge/platform/vulkan/vulkan_swapchain.h"
#include "sge/platform/vulkan/vulkan_context.h"
#include "sge/platform/vulkan/vulkan_render_pass.h"
namespace sge {
    static PFN_vkDestroySurfaceKHR fpDestroySurfaceKHR = nullptr;
    static PFN_vkCreateSwapchainKHR fpCreateSwapchainKHR = nullptr;
    static PFN_vkDestroySwapchainKHR fpDestroySwapchainKHR = nullptr;
    static PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR = nullptr;
    static PFN_vkQueuePresentKHR fpQueuePresentKHR = nullptr;
    static PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR = nullptr;

    static void load_swapchain_functions() {
        auto& context = vulkan_context::get();
        VkInstance instance = context.get_instance();
        VkDevice device = context.get_device().get();

#define LOAD_INSTANCE(name) fp##name = (PFN_vk##name)vkGetInstanceProcAddr(instance, "vk" #name)
#define LOAD_DEVICE(name) fp##name = (PFN_vk##name)vkGetDeviceProcAddr(device, "vk" #name)

        LOAD_INSTANCE(DestroySurfaceKHR);
        LOAD_DEVICE(CreateSwapchainKHR);
        LOAD_DEVICE(DestroySwapchainKHR);
        LOAD_DEVICE(AcquireNextImageKHR);
        LOAD_DEVICE(QueuePresentKHR);
        LOAD_DEVICE(GetSwapchainImagesKHR);

#undef LOAD_INSTANCE
#undef LOAD_DEVICE
    }

    vulkan_swapchain::vulkan_swapchain(ref<window> _window) {
        load_swapchain_functions();

        VkInstance instance = vulkan_context::get().get_instance();
        m_window = _window;
        m_surface = (VkSurfaceKHR)m_window->create_render_surface(instance);
        m_current_frame = 0;

        create(true);
        allocate_command_buffers();
        create_sync_objects();
    }

    vulkan_swapchain::~vulkan_swapchain() {
        auto& context = vulkan_context::get();
        VkInstance instance = context.get_instance();
        vulkan_device& device = context.get_device();
        VkDevice vk_device = device.get();
        auto physical_device = device.get_physical_device();

        vulkan_physical_device::queue_family_indices indices;
        physical_device.query_queue_families(VK_QUEUE_GRAPHICS_BIT, indices);
        VkQueue queue = device.get_queue(indices.graphics.value());
        vkQueueWaitIdle(queue);

        queue = device.get_queue(m_present_queue);
        vkQueueWaitIdle(queue);

        for (const auto& sync_objects_ : m_sync_objects) {
            vkDestroySemaphore(vk_device, sync_objects_.image_available, nullptr);
            vkDestroySemaphore(vk_device, sync_objects_.render_finished, nullptr);
            vkDestroyFence(vk_device, sync_objects_.fence, nullptr);
        }

        m_command_buffers.clear();
        vkDestroyCommandPool(vk_device, m_command_pool, nullptr);

        destroy();
        fpDestroySurfaceKHR(instance, m_surface, nullptr);
    }

    void vulkan_swapchain::on_resize(uint32_t new_width, uint32_t new_height) {
        m_new_size = glm::uvec2(new_width, new_height);
    }

    void vulkan_swapchain::new_frame() {
        VkDevice device = vulkan_context::get().get_device().get();
        VkFence fence = m_sync_objects[m_current_frame].fence;
        static constexpr size_t uint64_max = std::numeric_limits<uint64_t>::max();
        vkWaitForFences(device, 1, &fence, true, uint64_max);

        while (acquire_next_image()) {
            resize();
        }

        if (m_image_fences[m_current_image_index] != nullptr) {
            vkWaitForFences(device, 1, &m_image_fences[m_current_image_index], true, uint64_max);
        }
        m_image_fences[m_current_image_index] = fence;

        vkResetFences(device, 1, &fence);

        auto& cmdlist = *m_command_buffers[m_current_image_index];
        cmdlist.reset();
    }

    void vulkan_swapchain::present() {
        vulkan_device& device = vulkan_context::get().get_device();
        auto physical_device = device.get_physical_device();

        vulkan_physical_device::queue_family_indices families;
        physical_device.query_queue_families(VK_QUEUE_GRAPHICS_BIT, families);

        VkQueue graphics_queue = device.get_queue(families.graphics.value());
        VkQueue present_queue = device.get_queue(m_present_queue);
        const auto& sync_objects_ = m_sync_objects[m_current_frame];

        {
            auto& cmdlist = *m_command_buffers[m_current_image_index];
            VkCommandBuffer cmdbuffer = cmdlist.get();

            static VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            auto submit_info = vk_init<VkSubmitInfo>(VK_STRUCTURE_TYPE_SUBMIT_INFO);

            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &cmdbuffer;

            submit_info.waitSemaphoreCount = 1;
            submit_info.pWaitSemaphores = &sync_objects_.image_available;
            submit_info.pWaitDstStageMask = &wait_stage;

            submit_info.signalSemaphoreCount = 1;
            submit_info.pSignalSemaphores = &sync_objects_.render_finished;

            VkResult result = vkQueueSubmit(graphics_queue, 1, &submit_info, sync_objects_.fence);
            check_vk_result(result);
        }

        auto present_info = vk_init<VkPresentInfoKHR>(VK_STRUCTURE_TYPE_PRESENT_INFO_KHR);
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &sync_objects_.render_finished;

        present_info.swapchainCount = 1;
        present_info.pSwapchains = &m_swapchain;
        present_info.pImageIndices = &m_current_image_index;

        VkResult result = fpQueuePresentKHR(present_queue, &present_info);
        if (m_new_size.has_value() || result == VK_ERROR_OUT_OF_DATE_KHR ||
            result == VK_SUBOPTIMAL_KHR) {
            resize();
        } else {
            check_vk_result(result);
        }

        m_current_frame++;
        m_current_frame %= max_frames_in_flight;
    }

    bool vulkan_swapchain::acquire_next_image() {
        VkDevice device = vulkan_context::get().get_device().get();
        VkSemaphore semaphore = m_sync_objects[m_current_frame].image_available;
        VkResult result =
            fpAcquireNextImageKHR(device, m_swapchain, std::numeric_limits<uint64_t>::max(),
                                  semaphore, nullptr, &m_current_image_index);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            return true;
        } else if (result != VK_SUBOPTIMAL_KHR) {
            check_vk_result(result);
        }

        return false;
    }

    void vulkan_swapchain::allocate_command_buffers() {
        auto create_info =
            vk_init<VkCommandPoolCreateInfo>(VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
        create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VkDevice device = vulkan_context::get().get_device().get();
        VkResult result = vkCreateCommandPool(device, &create_info, nullptr, &m_command_pool);
        check_vk_result(result);

        for (size_t i = 0; i < m_swapchain_images.size(); i++) {
            auto cmdlist = std::make_unique<vulkan_command_list>(m_command_pool);
            m_command_buffers.push_back(std::move(cmdlist));
        }
    }

    void vulkan_swapchain::create_sync_objects() {
        VkDevice device = vulkan_context::get().get_device().get();

        auto semaphore_info =
            vk_init<VkSemaphoreCreateInfo>(VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);

        auto fence_info = vk_init<VkFenceCreateInfo>(VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < max_frames_in_flight; i++) {
            auto& data = m_sync_objects[i];

            VkResult result =
                vkCreateSemaphore(device, &semaphore_info, nullptr, &data.image_available);
            check_vk_result(result);

            result = vkCreateSemaphore(device, &semaphore_info, nullptr, &data.render_finished);
            check_vk_result(result);

            result = vkCreateFence(device, &fence_info, nullptr, &data.fence);
            check_vk_result(result);
        }
    }

    void vulkan_swapchain::resize() {
        vulkan_device& device = vulkan_context::get().get_device();
        auto physical_device = device.get_physical_device();

        vulkan_physical_device::queue_family_indices indices;
        physical_device.query_queue_families(VK_QUEUE_GRAPHICS_BIT, indices);
        VkQueue queue = device.get_queue(indices.graphics.value());
        vkQueueWaitIdle(queue);

        for (const auto& cmdlist : m_command_buffers) {
            cmdlist->reset();
        }

        destroy();
        create(false);
    }

    void vulkan_swapchain::create(bool render_pass) {
        create_swapchain();
        if (render_pass) {
            m_render_pass = ref<vulkan_render_pass>::create(this);
        }
        acquire_images();

        m_current_image_index = 0;
    }

    void vulkan_swapchain::destroy() {
        VkDevice device = vulkan_context::get().get_device().get();

        for (const auto& image : m_swapchain_images) {
            vkDestroyFramebuffer(device, image.framebuffer, nullptr);
            vkDestroyImageView(device, image.view, nullptr);
        }

        fpDestroySwapchainKHR(device, m_swapchain, nullptr);
    }

    static VkExtent2D choose_extent(uint32_t width, uint32_t height,
                                    const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            VkExtent2D min_extent = capabilities.minImageExtent;
            VkExtent2D max_extent = capabilities.maxImageExtent;

            VkExtent2D extent;
            extent.width = std::clamp(width, min_extent.width, max_extent.width);
            extent.height = std::clamp(height, min_extent.height, max_extent.height);
            return extent;
        }
    }

    static VkSurfaceFormatKHR choose_format(const std::vector<VkSurfaceFormatKHR>& formats) {
        static const std::vector<VkFormat> preferred_formats = { VK_FORMAT_B8G8R8A8_UNORM,
                                                                 VK_FORMAT_R8G8B8A8_UNORM,
                                                                 VK_FORMAT_B8G8R8_UNORM,
                                                                 VK_FORMAT_R8G8B8_UNORM };
        static constexpr VkColorSpaceKHR preferred_color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

        if (formats.size() == 1) {
            if (formats[0].format == VK_FORMAT_UNDEFINED) {
                VkSurfaceFormatKHR surface_format;

                surface_format.format = preferred_formats[0];
                surface_format.colorSpace = preferred_color_space;

                return surface_format;
            }
        } else {
            for (VkFormat preferred_format : preferred_formats) {
                for (const auto& format : formats) {
                    if (format.format == preferred_format &&
                        format.colorSpace == preferred_color_space) {
                        return format;
                    }
                }
            }
        }

        return formats[0];
    }

    static VkPresentModeKHR choose_present_mode(const std::vector<VkPresentModeKHR> present_modes) {
        static const std::vector<VkPresentModeKHR> preferred_present_modes{
            VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR
        };

        for (VkPresentModeKHR preferred_present_mode : preferred_present_modes) {
            for (VkPresentModeKHR present_mode : present_modes) {
                if (preferred_present_mode == present_mode) {
                    return present_mode;
                }
            }
        }

        return present_modes[0];
    }

    void vulkan_swapchain::create_swapchain() {
        vulkan_device& device = vulkan_context::get().get_device();
        auto physical_device = device.get_physical_device();
        VkPhysicalDevice vk_device = physical_device.get();

        vulkan_physical_device::queue_family_indices indices;
        physical_device.query_queue_families(VK_QUEUE_GRAPHICS_BIT, indices);

        VkSurfaceCapabilitiesKHR capabilities;
        physical_device.get_surface_capabilities(m_surface, capabilities);

        uint32_t image_count = capabilities.minImageCount;
        if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount) {
            image_count = capabilities.maxImageCount;
        }

        VkSurfaceFormatKHR format;
        {
            std::vector<VkSurfaceFormatKHR> surface_formats;
            physical_device.get_surface_formats(m_surface, surface_formats);
            format = choose_format(surface_formats);
        }

        VkPresentModeKHR present_mode;
        {
            std::vector<VkPresentModeKHR> present_modes;
            physical_device.get_surface_present_modes(m_surface, present_modes);
            present_mode = choose_present_mode(present_modes);
        }

        {
            auto present_queue = physical_device.find_surface_present_queue(m_surface);
            if (present_queue.has_value()) {
                m_present_queue = present_queue.value();
            } else {
                throw std::runtime_error("could not find a suitable presentation queue family");
            }
        }

        uint32_t width, height;
        if (m_new_size.has_value()) {
            width = m_new_size->x;
            height = m_new_size->y;
            m_new_size.reset();
        } else {
            width = m_window->get_width();
            height = m_window->get_height();
        }

        auto create_info =
            vk_init<VkSwapchainCreateInfoKHR>(VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR);

        create_info.surface = m_surface;
        create_info.minImageCount = image_count;
        create_info.presentMode = present_mode;

        m_image_format = format.format;
        create_info.imageFormat = format.format;
        create_info.imageColorSpace = format.colorSpace;

        VkExtent2D extent = choose_extent(width, height, capabilities);
        create_info.imageExtent = extent;
        m_width = extent.width;
        m_height = extent.height;

        create_info.imageArrayLayers = 1;
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        create_info.preTransform = capabilities.currentTransform;
        create_info.clipped = true;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        std::vector<uint32_t> queue_family_indices = { indices.graphics.value(), m_present_queue };

        if (indices.graphics != m_present_queue) {
            create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = queue_family_indices.size();
            create_info.pQueueFamilyIndices = queue_family_indices.data();
        } else {
            create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        VkResult result = fpCreateSwapchainKHR(device.get(), &create_info, nullptr, &m_swapchain);
        check_vk_result(result);
    }

    void vulkan_swapchain::acquire_images() {
        auto vk_render_pass = m_render_pass.as<vulkan_render_pass>();
        VkDevice device = vulkan_context::get().get_device().get();

        uint32_t image_count = 0;
        fpGetSwapchainImagesKHR(device, m_swapchain, &image_count, nullptr);
        std::vector<VkImage> images(image_count);
        fpGetSwapchainImagesKHR(device, m_swapchain, &image_count, images.data());

        m_swapchain_images.resize(image_count);
        m_image_fences.resize(image_count, nullptr);
        for (uint32_t i = 0; i < image_count; i++) {
            swapchain_image& data = m_swapchain_images[i];
            data.image = images[i];

            {
                auto create_info =
                    vk_init<VkImageViewCreateInfo>(VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);

                create_info.image = data.image;
                create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                create_info.format = m_image_format;

                create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

                create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                create_info.subresourceRange.baseMipLevel = 0;
                create_info.subresourceRange.levelCount = 1;
                create_info.subresourceRange.baseArrayLayer = 0;
                create_info.subresourceRange.layerCount = 1;

                VkResult result = vkCreateImageView(device, &create_info, nullptr, &data.view);
                check_vk_result(result);
            }

            {
                auto create_info =
                    vk_init<VkFramebufferCreateInfo>(VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);

                create_info.attachmentCount = 1;
                create_info.pAttachments = &data.view;
                create_info.renderPass = vk_render_pass->get();
                create_info.width = m_width;
                create_info.height = m_height;
                create_info.layers = 1;

                VkResult result =
                    vkCreateFramebuffer(device, &create_info, nullptr, &data.framebuffer);
                check_vk_result(result);
            }
        }
    }
} // namespace sge