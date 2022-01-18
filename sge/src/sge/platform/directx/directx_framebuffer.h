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
namespace sge {
    // dummy for now
    class directx_framebuffer : public framebuffer {
    public:
        directx_framebuffer(const framebuffer_spec& spec) { m_spec = spec; }
        virtual ~directx_framebuffer() override = default;

        virtual const framebuffer_spec& get_spec() override { return m_spec; }

        virtual uint32_t get_width() override { return 0; }
        virtual uint32_t get_height() override { return 0; }
        virtual void resize(uint32_t new_width, uint32_t new_height) override {}

        virtual ref<render_pass> get_render_pass() override { return nullptr; }

        virtual size_t get_attachment_count(framebuffer_attachment_type type) override { return 0; }
        virtual ref<image_2d> get_attachment(framebuffer_attachment_type type,
                                             size_t index) override {
            return nullptr;
        }

    private:
        framebuffer_spec m_spec;
    };
} // namespace sge
