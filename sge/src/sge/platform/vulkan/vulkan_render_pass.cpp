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
#include "sge/platform/vulkan/vulkan_render_pass.h"
#include "sge/platform/vulkan/vulkan_context.h"
#include "sge/platform/vulkan/vulkan_command_list.h"
#include "sge/platform/vulkan/vulkan_image.h"
namespace sge {
    vulkan_render_pass::vulkan_render_pass(vulkan_swapchain* parent) {
        this->m_swapchain_parent = parent;

        auto color_attachment = vk_init<VkAttachmentDescription>();
        color_attachment.format = parent->get_image_format();
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

        auto create_info =
            vk_init<VkRenderPassCreateInfo>(VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);

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

    vulkan_render_pass::vulkan_render_pass(vulkan_framebuffer* parent) {
        this->m_framebuffer_parent = parent;
        const auto& spec = parent->get_spec();

        std::vector<VkAttachmentDescription> attachment_descs;
        std::vector<VkAttachmentReference> color_attachments;
        std::unique_ptr<VkAttachmentReference> depth_attachment;

        std::set<framebuffer_attachment_type> types;
        parent->get_attachment_types(types);

        if (types.find(framebuffer_attachment_type::color) != types.end()) {
            size_t attachment_count =
                parent->get_attachment_count(framebuffer_attachment_type::color);

            for (uint32_t i = 0; (size_t)i < attachment_count; i++) {
                auto attachment = parent->get_attachment(framebuffer_attachment_type::color, i);
                auto vk_image = attachment.as<vulkan_image_2d>();

                auto attachment_ref = vk_init<VkAttachmentReference>();
                attachment_ref.attachment = i;
                attachment_ref.layout = vk_image->get_layout();
                color_attachments.push_back(attachment_ref);

                auto attachment_desc = vk_init<VkAttachmentDescription>();
                attachment_desc.format = vk_image->get_vulkan_format();
                attachment_desc.finalLayout = vk_image->get_layout();
                attachment_desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                attachment_desc.samples = VK_SAMPLE_COUNT_1_BIT;
                attachment_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachment_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachment_desc.loadOp =
                    spec.clear_on_load ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
                attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachment_descs.push_back(attachment_desc);
            }
        }

        // todo(nora): depth attachment

        // also maybe subpass dependencies

        auto subpass = vk_init<VkSubpassDescription>();
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        if (!color_attachments.empty()) {
            subpass.colorAttachmentCount = color_attachments.size();
            subpass.pColorAttachments = color_attachments.data();
        }

        if (depth_attachment) {
            subpass.pDepthStencilAttachment = depth_attachment.get();
        }

        auto create_info =
            vk_init<VkRenderPassCreateInfo>(VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);
        create_info.subpassCount = 1;
        create_info.pSubpasses = &subpass;
        create_info.attachmentCount = attachment_descs.size();
        create_info.pAttachments = attachment_descs.data();

        VkDevice device = vulkan_context::get().get_device().get();
        VkResult result = vkCreateRenderPass(device, &create_info, nullptr, &this->m_render_pass);
        check_vk_result(result);
    }

    vulkan_render_pass::~vulkan_render_pass() {
        VkDevice device = vulkan_context::get().get_device().get();
        vkDestroyRenderPass(device, this->m_render_pass, nullptr);
    }

    render_pass_parent_type vulkan_render_pass::get_parent_type() {
        if (this->m_swapchain_parent != nullptr) {
            return render_pass_parent_type::swapchain;
        } else if (this->m_framebuffer_parent != nullptr) {
            return render_pass_parent_type::framebuffer;
        }

        throw std::runtime_error("what lmao");
    }

    void vulkan_render_pass::begin(command_list& cmdlist, const glm::vec4& clear_color) {
        VkExtent2D extent;
        VkFramebuffer fb;
        if (this->m_swapchain_parent != nullptr) {
            extent = { this->m_swapchain_parent->get_width(),
                       this->m_swapchain_parent->get_height() };

            size_t current_image = this->m_swapchain_parent->get_current_image_index();
            fb = this->m_swapchain_parent->get_framebuffer(current_image);
        } else if (this->m_framebuffer_parent != nullptr) {
            extent = { this->m_framebuffer_parent->get_width(),
                       this->m_framebuffer_parent->get_height() };
            fb = this->m_framebuffer_parent->get();
        } else {
            throw std::runtime_error("this should not be hit");
        }

        auto& vk_cmdlist = (vulkan_command_list&)cmdlist;
        VkCommandBuffer cmdbuffer = vk_cmdlist.get();

        auto begin_info = vk_init<VkRenderPassBeginInfo>(VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO);

        VkRect2D render_area;
        render_area.extent = extent;
        render_area.offset = { 0, 0 };
        begin_info.renderArea = render_area;

        auto clear_value = vk_init<VkClearValue>();
        memcpy(clear_value.color.float32, &clear_color, sizeof(float) * 4);
        begin_info.clearValueCount = 1;
        begin_info.pClearValues = &clear_value;

        begin_info.framebuffer = fb;
        begin_info.renderPass = this->m_render_pass;

        vkCmdBeginRenderPass(cmdbuffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport;
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;

        viewport.x = 0;
        viewport.width = (float)extent.width;

        viewport.y = (float)extent.height;
        viewport.height = -viewport.y;
        vkCmdSetViewport(cmdbuffer, 0, 1, &viewport);

        VkRect2D scissor;
        scissor.offset = { 0, 0 };
        scissor.extent = extent;
        vkCmdSetScissor(cmdbuffer, 0, 1, &scissor);
    } // namespace sge

    void vulkan_render_pass::end(command_list& cmdlist) {
        auto& vk_cmdlist = (vulkan_command_list&)cmdlist;
        VkCommandBuffer cmdbuffer = vk_cmdlist.get();
        vkCmdEndRenderPass(cmdbuffer);
    }
} // namespace sge