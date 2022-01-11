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
#include "sge/platform/vulkan/vulkan_framebuffer.h"
#include "sge/platform/vulkan/vulkan_render_pass.h"
#include "sge/platform/vulkan/vulkan_context.h"
namespace sge {
    vulkan_framebuffer::vulkan_framebuffer(const framebuffer_spec& spec) {
        m_spec = spec;
        m_width = m_spec.width;
        m_height = m_spec.height;

        acquire_attachments();
        m_render_pass = ref<vulkan_render_pass>::create(this);
        create();
    }

    vulkan_framebuffer::~vulkan_framebuffer() { destroy(); }

    void vulkan_framebuffer::resize(uint32_t new_width, uint32_t new_height) {
        destroy();

        m_width = new_width;
        m_height = new_height;

        acquire_attachments();
        create();
    }

    void vulkan_framebuffer::get_attachment_types(std::set<framebuffer_attachment_type>& types) {
        for (const auto& [type, attachments] : m_attachments) {
            types.insert(type);
        }
    }

    size_t vulkan_framebuffer::get_attachment_count(framebuffer_attachment_type type) {
        if (m_attachments.find(type) == m_attachments.end()) {
            return 0;
        }

        return m_attachments[type].size();
    }

    ref<image_2d> vulkan_framebuffer::get_attachment(framebuffer_attachment_type type,
                                                     size_t index) {
        if (m_attachments.find(type) == m_attachments.end()) {
            return nullptr;
        }

        return m_attachments[type][index];
    }

    void vulkan_framebuffer::acquire_attachments() {
        if (!m_attachments.empty()) {
            m_attachments.clear();
        }

        for (const auto& spec : m_spec.attachments) {
            image_spec img_spec;
            img_spec.mip_levels = 1;
            img_spec.array_layers = 1;
            img_spec.width = m_width;
            img_spec.height = m_height;
            img_spec.format = spec.format;
            img_spec.image_usage = image_usage_attachment | spec.additional_usage;
            auto img = ref<vulkan_image_2d>::create(img_spec);

            VkImageLayout optimal_layout;
            if (spec.additional_usage != image_usage_none) {
                optimal_layout = VK_IMAGE_LAYOUT_GENERAL;
            } else {
                // todo(nora): depth attachment?
                optimal_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            }

            if (img->get_layout() != optimal_layout) {
                img->set_layout(optimal_layout);
            }

            m_attachments[spec.type].push_back(img);
        }
    }

    void vulkan_framebuffer::destroy() {
        VkDevice device = vulkan_context::get().get_device().get();
        vkDestroyFramebuffer(device, m_framebuffer, nullptr);
    }

    void vulkan_framebuffer::create() {
        std::vector<VkImageView> views;
        for (const auto& [type, attachments] : m_attachments) {
            for (auto img : attachments) {
                views.push_back(img->get_view());
            }
        }

        auto create_info =
            vk_init<VkFramebufferCreateInfo>(VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);

        create_info.renderPass = m_render_pass.as<vulkan_render_pass>()->get();
        create_info.attachmentCount = views.size();
        create_info.pAttachments = views.data();
        create_info.width = m_width;
        create_info.height = m_height;
        create_info.layers = 1;
        
        VkDevice device = vulkan_context::get().get_device().get();
        VkResult result = vkCreateFramebuffer(device, &create_info, nullptr, &m_framebuffer);
        check_vk_result(result);
    }
} // namespace sge