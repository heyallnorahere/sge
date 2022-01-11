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
#include "sge/renderer/framebuffer.h"
#include "sge/platform/vulkan/vulkan_image.h"
namespace sge {
    class vulkan_framebuffer : public framebuffer {
    public:
        vulkan_framebuffer(const framebuffer_spec& spec);
        virtual ~vulkan_framebuffer() override;

        virtual const framebuffer_spec& get_spec() override { return m_spec; }

        virtual uint32_t get_width() override { return m_width; }
        virtual uint32_t get_height() override { return m_height; }
        virtual void resize(uint32_t new_width, uint32_t new_height) override;

        virtual ref<render_pass> get_render_pass() override { return m_render_pass; }

        void get_attachment_types(std::set<framebuffer_attachment_type>& types);
        virtual size_t get_attachment_count(framebuffer_attachment_type type) override;
        virtual ref<image_2d> get_attachment(framebuffer_attachment_type type,
                                             size_t index) override;

        VkFramebuffer get() { return m_framebuffer; }

    private:
        void acquire_attachments();
        void destroy();
        void create();

        VkFramebuffer m_framebuffer;
        std::map<framebuffer_attachment_type, std::vector<ref<vulkan_image_2d>>> m_attachments;
        ref<render_pass> m_render_pass;
        uint32_t m_width, m_height;
        framebuffer_spec m_spec;
    };
} // namespace sge