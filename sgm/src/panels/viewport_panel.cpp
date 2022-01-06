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

#include "sgmpch.h"
#include "panels/panels.h"
#include "editor_scene.h"
#include <sge/renderer/renderer.h>
namespace sgm {
    void viewport_panel::update(timestep ts) {
        if (m_new_size.has_value()) {
            renderer::wait();

            auto fb = editor_scene::get_framebuffer();

            glm::uvec2 size = m_new_size.value();
            fb->resize(size.x, size.y);
            invalidate_texture();

            m_new_size.reset();
        }
    }

    void viewport_panel::begin(const char* title, bool* open) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
        ImGui::Begin(title, open);
        ImGui::PopStyleVar();
    }

    void viewport_panel::render() {
        if (!m_current_texture) {
            invalidate_texture();
        }
        verify_size();

        ImVec2 content_region = ImGui::GetContentRegionAvail();
        ImGui::Image(m_current_texture->get_imgui_id(), content_region);
    }

    void viewport_panel::verify_size() {
        glm::uvec2 current_size, fb_size;

        ImVec2 content_region = ImGui::GetContentRegionAvail();
        current_size.x = (uint32_t)content_region.x;
        current_size.y = (uint32_t)content_region.y;

        auto fb = editor_scene::get_framebuffer();
        fb_size.x = fb->get_width();
        fb_size.y = fb->get_height();

        if (current_size != fb_size) {
            m_new_size = current_size;
        }
    }

    void viewport_panel::invalidate_texture() {
        auto fb = editor_scene::get_framebuffer();
        auto attachment = fb->get_attachment(framebuffer_attachment_type::color, 0);

        swapchain& swap_chain = application::get().get_swapchain();
        size_t current_image = swap_chain.get_current_image_index();

        if (m_old_textures.empty()) {
            size_t image_count = swap_chain.get_image_count();
            m_old_textures.resize(image_count);
        }
        m_old_textures[current_image] = m_current_texture;
        
        texture_2d_spec spec;
        spec.image = attachment;
        spec.filter = texture_filter::linear;
        spec.wrap = texture_wrap::repeat;
        m_current_texture = texture_2d::create(spec);
    }
}