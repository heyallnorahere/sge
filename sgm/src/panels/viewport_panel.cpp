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
    viewport_panel::viewport_panel(
        const std::function<void(const fs::path&)>& load_scene_callback) {
        m_load_scene_callback = load_scene_callback;
    }

    void viewport_panel::update(timestep ts) {
        if (m_new_size.has_value()) {
            renderer::wait();

            glm::uvec2 size = m_new_size.value();
            editor_scene::set_viewport_size(size.x, size.y);
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

        ImGuiID viewport_id = ImGui::GetID("viewport-image");
        ImGui::PushID(viewport_id);

        ImVec2 content_region = ImGui::GetContentRegionAvail();
        ImGui::Image(m_current_texture->get_imgui_id(), content_region);

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("scene")) {
                fs::path path =
                    std::string((const char*)payload->Data, payload->DataSize / sizeof(char) - 1);

                fs::path asset_dir = project::get().get_asset_dir();
                fs::path asset_path = asset_dir / path;
                editor_scene::load(asset_path);

                if (m_load_scene_callback) {
                    m_load_scene_callback(asset_path);
                }
            }

            ImGui::EndDragDropTarget();
        }

        if (ImGui::IsItemHovered()) {
            editor_scene::enable_input();
        } else if (!input::get_mouse_button(mouse_button::right)) {
            editor_scene::disable_input();
        }

        ImGui::PopID();
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

        texture_spec spec;
        spec.image = attachment;
        spec.filter = texture_filter::linear;
        spec.wrap = texture_wrap::repeat;
        m_current_texture = texture_2d::create(spec);
    }
} // namespace sgm
