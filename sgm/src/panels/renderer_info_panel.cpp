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
    void renderer_info_panel::update(timestep ts) {
        if (m_reload_shaders) {
            renderer::wait();

            auto& library = renderer::get_shader_library();
            library.reload_all();

            m_reload_shaders = false;
        }
    }

    void renderer_info_panel::render() {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("%f FPS", io.Framerate);

        if (ImGui::Button("Reload library shaders")) {
            m_reload_shaders = true;
        }

        if (ImGui::CollapsingHeader("Renderer stats")) {
            renderer::stats stats = renderer::get_stats();

            ImGui::Text("Draw calls: %u", stats.draw_calls);
            ImGui::Text("Quads: %u", stats.quad_count);
            ImGui::Text("Vertices: %u", stats.vertex_count);
            ImGui::Text("Indices: %u", stats.index_count);
        }

        if (ImGui::CollapsingHeader("Device info")) {
            device_info info = renderer::query_device_info();

            ImGui::Text("Device: %s", info.name.c_str());
            ImGui::Text("API: %s", info.graphics_api.c_str());
        }

        if (ImGui::CollapsingHeader("Camera data")) {
            editor_camera& camera = editor_scene::get_camera();

            glm::vec2 position = camera.get_position();
            ImGui::Text("Position: (%f, %f)", position.x, position.y);
            ImGui::Text("View size: %f", camera.get_view_size());
        }
    }
} // namespace sgm
