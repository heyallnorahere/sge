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
#include "sge/renderer/render_pass.h"
#include "sge/renderer/image.h"
namespace sge {
    enum class framebuffer_blend_mode {
        none,
        one_zero,
        src_alpha_one_minus_src_alpha,
        zero_src_color
    };

    enum class framebuffer_attachment_type { color };

    struct framebuffer_attachment_spec {
        framebuffer_attachment_type type;
        image_format format;
        uint32_t additional_usage = image_usage_none;
    };

    struct framebuffer_spec {
        uint32_t width = 0;
        uint32_t height = 0;

        bool clear_on_load = true;
        std::vector<framebuffer_attachment_spec> attachments;

        bool enable_blending = false;
        framebuffer_blend_mode blend_mode = framebuffer_blend_mode::none;

        // todo(nora): possibly render pass sharing?
    };

    class framebuffer : public ref_counted {
    public:
        static ref<framebuffer> create(const framebuffer_spec& spec);

        virtual ~framebuffer() = default;

        virtual const framebuffer_spec& get_spec() = 0;

        virtual uint32_t get_width() = 0;
        virtual uint32_t get_height() = 0;
        virtual void resize(uint32_t new_width, uint32_t new_height) = 0;

        virtual ref<render_pass> get_render_pass() = 0;

        virtual size_t get_attachment_count(framebuffer_attachment_type type) = 0;
        virtual ref<image_2d> get_attachment(framebuffer_attachment_type type, size_t index) = 0;
    };
} // namespace sge