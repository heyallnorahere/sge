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
    vulkan_swapchain::vulkan_swapchain(window& _window) {
        VkInstance instance = vulkan_context::get().get_instance();
        this->m_surface = (VkSurfaceKHR)_window.create_render_surface(instance);

        this->create();
        this->create_render_pass();
        this->allocate_command_buffers();
        this->create_sync_objects();

        {
            static VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            this->m_submit_info = vk_init<VkSubmitInfo>(VK_STRUCTURE_TYPE_SUBMIT_INFO);
            this->m_submit_info.commandBufferCount = 1;

            this->m_submit_info.waitSemaphoreCount = 1;
            this->m_submit_info.pWaitSemaphores = &this->m_semphores.image_available;
            this->m_submit_info.pWaitDstStageMask = &wait_stage;

            this->m_submit_info.signalSemaphoreCount = 1;
            this->m_submit_info.pSignalSemaphores = &this->m_semphores.render_finished;
        }
    }

    vulkan_swapchain::~vulkan_swapchain() {
        auto& context = vulkan_context::get();
        VkInstance instance = context.get_instance();
        VkDevice device = context.get_device().get();

        vkDestroyRenderPass(device, this->m_render_pass, nullptr);
        this->destroy();
        vkDestroySurfaceKHR(instance, this->m_surface, nullptr);
    }

    void vulkan_swapchain::on_resize(uint32_t new_width, uint32_t new_height) {
        this->m_new_size = glm::uvec2(new_width, new_height);
    }

    void vulkan_swapchain::new_frame() {
        while (this->acquire_next_image()) { this->resize(); }

        VkDevice device = vulkan_context::get().get_device().get();
        VkFence fence = this->m_fences[this->m_current_image_index];
        vkWaitForFences(device, 1, &fence, true, std::numeric_limits<uint64_t>::max());
        vkResetFences(device, 1, &fence);
    }

    void vulkan_swapchain::present() {
        vulkan_device& device = vulkan_context::get().get_device();
        auto physical_device = device.get_physical_device();
        
        vulkan_physical_device::queue_family_indices families;
        physical_device.query_queue_families(VK_QUEUE_GRAPHICS_BIT, families);

        VkQueue graphics_queue = device.get_queue(families.graphics.value());
        VkQueue present_queue = device.get_queue(this->m_present_queue);

        VkCommandBuffer cmdbuffer = this->m_command_buffers[this->m_current_image_index]->get();
        VkFence fence = this->m_fences[this->m_current_image_index];
        this->m_submit_info.pCommandBuffers = &cmdbuffer;
        VkResult result = vkQueueSubmit(graphics_queue, 1, &this->m_submit_info, fence);
        check_vk_result(result);

        auto present_info = vk_init<VkPresentInfoKHR>(VK_STRUCTURE_TYPE_PRESENT_INFO_KHR);
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &this->m_semphores.render_finished;
        
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &this->m_swapchain;
        present_info.pImageIndices = &this->m_current_image_index;

        while (this->present_queue(present_queue, present_info)) { this->resize(); }
    }

    void vulkan_swapchain::begin(ref<command_list> cmdlist, const glm::vec4& clear_color) {
        auto vk_cmdlist = cmdlist.as<vulkan_command_list>();
        VkCommandBuffer cmdbuffer = vk_cmdlist->get();

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

    void vulkan_swapchain::end(ref<command_list> cmdlist) {
        auto vk_cmdlist = cmdlist.as<vulkan_command_list>();
        VkCommandBuffer cmdbuffer = vk_cmdlist->get();
        vkCmdEndRenderPass(cmdbuffer);
    }

    bool vulkan_swapchain::acquire_next_image() {
        VkDevice device = vulkan_context::get().get_device().get();
        VkResult result = vkAcquireNextImageKHR(device, this->m_swapchain,
            std::numeric_limits<uint64_t>::max(), this->m_semphores.image_available, nullptr,
            &this->m_current_image_index);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            return true;
        } else if (result != VK_SUBOPTIMAL_KHR) {
            check_vk_result(result);
        }

        return false;
    }

    bool vulkan_swapchain::present_queue(VkQueue queue, const VkPresentInfoKHR& present_info) {
        VkResult result = vkQueuePresentKHR(queue, &present_info);

        if (this->m_new_size.has_value() || result == VK_ERROR_OUT_OF_DATE_KHR ||
            result == VK_SUBOPTIMAL_KHR) {
            return true;
        } else {
            check_vk_result(result);
        }

        return false;
    }

    void vulkan_swapchain::create_render_pass() {
        // todo: create render pass
    }

    void vulkan_swapchain::allocate_command_buffers() {
        // todo: create command pool and allocate command buffers
    }

    void vulkan_swapchain::create_sync_objects() {
        // todo: create semaphores and fences
    }

    void vulkan_swapchain::resize() {
        this->destroy();
        this->create();
    }

    void vulkan_swapchain::create() {
        this->create_swapchain();
        this->acquire_images();
    }

    void vulkan_swapchain::destroy() {
        VkDevice device = vulkan_context::get().get_device().get();

        for (const auto& image : this->m_swapchain_images) {
            vkDestroyFramebuffer(device, image.framebuffer, nullptr);
            vkDestroyImageView(device, image.view, nullptr);
        }

        vkDestroySwapchainKHR(device, this->m_swapchain, nullptr);
    }

    void vulkan_swapchain::create_swapchain() {
        // todo: create swapchain
    }

    void vulkan_swapchain::acquire_images() {
        // todo: acquire images and create views and framebuffers
    }
}