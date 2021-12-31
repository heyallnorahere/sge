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
#include "sge/platform/vulkan/vulkan_swapchain.h"
#include "sge/platform/vulkan/vulkan_context.h"
namespace sge {
    vulkan_swapchain::vulkan_swapchain(ref<window> _window) {
        VkInstance instance = vulkan_context::get().get_instance();
        this->m_window = _window;
        this->m_surface = (VkSurfaceKHR)this->m_window->create_render_surface(instance);
        this->m_current_frame = 0;

        this->create(true);
        this->allocate_command_buffers();
        this->create_sync_objects();
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

        queue = device.get_queue(this->m_present_queue);
        vkQueueWaitIdle(queue);

        for (const auto& sync_objects_ : this->m_sync_objects) {
            vkDestroySemaphore(vk_device, sync_objects_.image_available, nullptr);
            vkDestroySemaphore(vk_device, sync_objects_.render_finished, nullptr);
            vkDestroyFence(vk_device, sync_objects_.fence, nullptr);
        }

        this->m_command_buffers.clear();
        vkDestroyCommandPool(vk_device, this->m_command_pool, nullptr);

        vkDestroyRenderPass(vk_device, this->m_render_pass, nullptr);
        this->destroy();
        vkDestroySurfaceKHR(instance, this->m_surface, nullptr);
    }

    void vulkan_swapchain::on_resize(uint32_t new_width, uint32_t new_height) {
        this->m_new_size = glm::uvec2(new_width, new_height);
    }

    void vulkan_swapchain::new_frame() {
        VkDevice device = vulkan_context::get().get_device().get();
        VkFence fence = this->m_sync_objects[this->m_current_frame].fence;
        static constexpr size_t uint64_max = std::numeric_limits<uint64_t>::max();
        vkWaitForFences(device, 1, &fence, true, uint64_max);
        
        while (this->acquire_next_image()) { this->resize(); }

        if (this->m_image_fences[this->m_current_image_index] != nullptr) {
            vkWaitForFences(device, 1, &this->m_image_fences[this->m_current_image_index],
                true, uint64_max);
        }
        this->m_image_fences[this->m_current_image_index] = fence;

        vkResetFences(device, 1, &fence);

        auto& cmdlist = *this->m_command_buffers[this->m_current_image_index];
        cmdlist.reset();
    }

    void vulkan_swapchain::present() {
        vulkan_device& device = vulkan_context::get().get_device();
        auto physical_device = device.get_physical_device();
        
        vulkan_physical_device::queue_family_indices families;
        physical_device.query_queue_families(VK_QUEUE_GRAPHICS_BIT, families);

        VkQueue graphics_queue = device.get_queue(families.graphics.value());
        VkQueue present_queue = device.get_queue(this->m_present_queue);
        const auto& sync_objects_ = this->m_sync_objects[this->m_current_frame];

        {
            auto& cmdlist = *this->m_command_buffers[this->m_current_image_index];
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
        present_info.pSwapchains = &this->m_swapchain;
        present_info.pImageIndices = &this->m_current_image_index;

        VkResult result = vkQueuePresentKHR(present_queue, &present_info);
        if (this->m_new_size.has_value() || result == VK_ERROR_OUT_OF_DATE_KHR ||
            result == VK_SUBOPTIMAL_KHR) {
            this->resize();
        } else {
            check_vk_result(result);
        }

        this->m_current_frame++;
        this->m_current_frame %= max_frames_in_flight;
    }

    void vulkan_swapchain::begin(command_list& cmdlist, const glm::vec4& clear_color) {
        auto& vk_cmdlist = (vulkan_command_list&)cmdlist;
        VkCommandBuffer cmdbuffer = vk_cmdlist.get();

        auto begin_info = vk_init<VkRenderPassBeginInfo>(VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO);

        VkRect2D render_area;
        render_area.extent = { this->m_width, this->m_height };
        render_area.offset = { 0, 0 };
        begin_info.renderArea = render_area;

        auto clear_value = vk_init<VkClearValue>();
        memcpy(clear_value.color.float32, &clear_color, sizeof(float) * 4);
        begin_info.clearValueCount = 1;
        begin_info.pClearValues = &clear_value;

        begin_info.framebuffer = this->m_swapchain_images[this->m_current_image_index].framebuffer;
        begin_info.renderPass = this->m_render_pass;

        vkCmdBeginRenderPass(cmdbuffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
    }

    void vulkan_swapchain::end(command_list& cmdlist) {
        auto& vk_cmdlist = (vulkan_command_list&)cmdlist;
        VkCommandBuffer cmdbuffer = vk_cmdlist.get();
        vkCmdEndRenderPass(cmdbuffer);
    }

    bool vulkan_swapchain::acquire_next_image() {
        VkDevice device = vulkan_context::get().get_device().get();
        VkSemaphore semaphore = this->m_sync_objects[this->m_current_frame].image_available;
        VkResult result = vkAcquireNextImageKHR(device, this->m_swapchain,
            std::numeric_limits<uint64_t>::max(), semaphore, nullptr,
            &this->m_current_image_index);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            return true;
        } else if (result != VK_SUBOPTIMAL_KHR) {
            check_vk_result(result);
        }

        return false;
    }

    void vulkan_swapchain::create_render_pass() {
        auto color_attachment = vk_init<VkAttachmentDescription>();
        color_attachment.format = this->m_image_format;
        color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        auto color_attachment_ref = vk_init<VkAttachmentReference>();
        color_attachment_ref.attachment = 0;
        color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        auto subpass = vk_init<VkSubpassDescription>();
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment_ref;

        auto dependency = vk_init<VkSubpassDependency>();

        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;

        dependency.srcStageMask = dependency.dstStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        dependency.srcAccessMask = 0;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        auto create_info = vk_init<VkRenderPassCreateInfo>(
            VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);

        create_info.attachmentCount = 1;
        create_info.pAttachments = &color_attachment;

        create_info.subpassCount = 1;
        create_info.pSubpasses = &subpass;

        create_info.dependencyCount = 1;
        create_info.pDependencies = &dependency;

        VkDevice device = vulkan_context::get().get_device().get();
        VkResult result = vkCreateRenderPass(device, &create_info, nullptr, &this->m_render_pass);
        check_vk_result(result);
    }

    void vulkan_swapchain::allocate_command_buffers() {
        auto create_info = vk_init<VkCommandPoolCreateInfo>(
            VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
        create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        
        VkDevice device = vulkan_context::get().get_device().get();
        VkResult result = vkCreateCommandPool(device, &create_info,
            nullptr, &this->m_command_pool);
        check_vk_result(result);

        for (size_t i = 0; i < this->m_swapchain_images.size(); i++) {
            auto cmdlist = std::make_unique<vulkan_command_list>(this->m_command_pool);
            this->m_command_buffers.push_back(std::move(cmdlist));
        }
    }

    void vulkan_swapchain::create_sync_objects() {
        VkDevice device = vulkan_context::get().get_device().get();

        auto semaphore_info = vk_init<VkSemaphoreCreateInfo>(
            VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
        
        auto fence_info = vk_init<VkFenceCreateInfo>(VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < max_frames_in_flight; i++) {
            auto& data = this->m_sync_objects[i];

            VkResult result = vkCreateSemaphore(device, &semaphore_info,
                nullptr, &data.image_available);
            check_vk_result(result);

            result = vkCreateSemaphore(device, &semaphore_info,
                nullptr, &data.render_finished);
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

        for (const auto& cmdlist : this->m_command_buffers) {
            cmdlist->reset();
        }

        this->destroy();
        this->create(false);
    }

    void vulkan_swapchain::create(bool render_pass) {
        this->create_swapchain();
        if (render_pass) {
            this->create_render_pass();
        }
        this->acquire_images();

        this->m_current_image_index = 0;
    }

    void vulkan_swapchain::destroy() {
        VkDevice device = vulkan_context::get().get_device().get();

        for (const auto& image : this->m_swapchain_images) {
            vkDestroyFramebuffer(device, image.framebuffer, nullptr);
            vkDestroyImageView(device, image.view, nullptr);
        }

        vkDestroySwapchainKHR(device, this->m_swapchain, nullptr);
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
        static const std::vector<VkFormat> preferred_formats = {
            VK_FORMAT_B8G8R8A8_UNORM,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_FORMAT_B8G8R8_UNORM,
            VK_FORMAT_R8G8B8_UNORM
        };
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

    static VkPresentModeKHR choose_present_mode(
        const std::vector<VkPresentModeKHR> present_modes) {
        static const std::vector<VkPresentModeKHR> preferred_present_modes {
            VK_PRESENT_MODE_MAILBOX_KHR,
            VK_PRESENT_MODE_IMMEDIATE_KHR,
            VK_PRESENT_MODE_FIFO_KHR
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
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_device, this->m_surface, &capabilities);

        uint32_t image_count = capabilities.minImageCount;
        if (capabilities.maxImageCount > 0 &&
            image_count > capabilities.maxImageCount) {
            image_count = capabilities.maxImageCount;
        }

        VkSurfaceFormatKHR format;
        {
            uint32_t surface_format_count = 0;
            vkGetPhysicalDeviceSurfaceFormatsKHR(vk_device, this->m_surface,
                &surface_format_count, nullptr);

            std::vector<VkSurfaceFormatKHR> surface_formats;
            if (surface_format_count > 0) {
                surface_formats.resize(surface_format_count);
                vkGetPhysicalDeviceSurfaceFormatsKHR(vk_device, this->m_surface,
                    &surface_format_count, surface_formats.data());
            }

            format = choose_format(surface_formats);
        }

        VkPresentModeKHR present_mode;
        {
            uint32_t present_mode_count = 0;
            vkGetPhysicalDeviceSurfacePresentModesKHR(vk_device, this->m_surface,
                &present_mode_count, nullptr);
            
            std::vector<VkPresentModeKHR> present_modes;
            if (present_mode_count > 0) {
                present_modes.resize(present_mode_count);
                vkGetPhysicalDeviceSurfacePresentModesKHR(vk_device, this->m_surface,
                    &present_mode_count, present_modes.data());
            }

            present_mode = choose_present_mode(present_modes);
        }

        {
            std::optional<uint32_t> present_queue;
            
            uint32_t family_count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(vk_device, &family_count, nullptr);

            for (uint32_t i = 0; i < family_count; i++) {
                VkBool32 presentation_supported = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(vk_device, i, this->m_surface,
                    &presentation_supported);
                
                if (presentation_supported) {
                    present_queue = i;
                    break;
                }
            }

            if (present_queue.has_value()) {
                this->m_present_queue = present_queue.value();
            } else {
                throw std::runtime_error("could not find a suitable presentation queue family");
            }
        }

        uint32_t width, height;
        if (this->m_new_size.has_value()) {
            width = this->m_new_size->x;
            height = this->m_new_size->y;
            this->m_new_size.reset();
        } else {
            width = this->m_window->get_width();
            height = this->m_window->get_height();
        }

        auto create_info = vk_init<VkSwapchainCreateInfoKHR>(
            VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR);
        
        create_info.surface = this->m_surface;
        create_info.minImageCount = image_count;
        create_info.presentMode = present_mode;

        this->m_image_format = format.format;
        create_info.imageFormat = format.format;
        create_info.imageColorSpace = format.colorSpace;

        VkExtent2D extent = choose_extent(width, height, capabilities);
        create_info.imageExtent = extent;
        this->m_width = extent.width;
        this->m_height = extent.height;

        create_info.imageArrayLayers = 1;
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        create_info.preTransform = capabilities.currentTransform;
        create_info.clipped = true;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        
        std::vector<uint32_t> queue_family_indices = {
            indices.graphics.value(),
            this->m_present_queue
        };

        if (indices.graphics != this->m_present_queue) {
            create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = queue_family_indices.size();
            create_info.pQueueFamilyIndices = queue_family_indices.data();
        } else {
            create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        VkResult result = vkCreateSwapchainKHR(device.get(), &create_info,
            nullptr, &this->m_swapchain);
        check_vk_result(result);
    }

    void vulkan_swapchain::acquire_images() {
        VkDevice device = vulkan_context::get().get_device().get();

        uint32_t image_count = 0;
        vkGetSwapchainImagesKHR(device, this->m_swapchain, &image_count, nullptr);
        std::vector<VkImage> images(image_count);
        vkGetSwapchainImagesKHR(device, this->m_swapchain, &image_count, images.data());

        this->m_swapchain_images.resize(image_count);
        this->m_image_fences.resize(image_count, nullptr);
        for (uint32_t i = 0; i < image_count; i++) {
            swapchain_image& data = this->m_swapchain_images[i];
            data.image = images[i];

            {
                auto create_info = vk_init<VkImageViewCreateInfo>(
                    VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
                
                create_info.image = data.image;
                create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                create_info.format = this->m_image_format;

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
                auto create_info = vk_init<VkFramebufferCreateInfo>(
                    VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);
                
                create_info.attachmentCount = 1;
                create_info.pAttachments = &data.view;
                create_info.renderPass = this->m_render_pass;
                create_info.width = this->m_width;
                create_info.height = this->m_height;
                create_info.layers = 1;

                VkResult result = vkCreateFramebuffer(device, &create_info, nullptr,
                    &data.framebuffer);
                check_vk_result(result);
            }
        }
    }
}