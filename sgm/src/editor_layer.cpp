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
#include "editor_layer.h"
#include "editor_scene.h"
#include "texture_cache.h"
namespace sgm {
    void editor_layer::on_update(timestep ts) {
        for (auto& _panel : m_panels) {
            _panel->update(ts);
        }

        editor_scene::on_update(ts);
    }

    void editor_layer::on_event(event& e) { editor_scene::on_event(e); }

    void editor_layer::on_imgui_render() {
        static constexpr ImGuiConfigFlags required_flags =
            ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;

        ImGuiIO& io = ImGui::GetIO();
        if ((io.ConfigFlags & required_flags) != required_flags) {
            throw std::runtime_error("SGM cannot be run in this environment.");
        }

        texture_cache::new_frame();
        update_dockspace();

        for (auto& _panel : m_panels) {
            if (_panel->open()) {
                // begin imgui window
                std::string title = _panel->get_title();
                _panel->begin(title.c_str(), &_panel->open());

                // render the panel
                _panel->render();

                // end imgui window
                ImGui::End();
            }
        }
    }

    void editor_layer::update_dockspace() {
        static constexpr ImGuiWindowFlags window_flags =
            ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

        ImGui::Begin("SGM Dockspace", nullptr, window_flags);

        ImGui::PopStyleVar(3);

        ImGui::DockSpace(ImGui::GetID("sgm-dockspace"));

        menu_bar();

        ImGui::End();
    }

    void editor_layer::menu_bar() {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Exit")) {
                    auto& app = application::get();
                    app.quit();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                for (auto& _panel : m_panels) {
                    std::string title = _panel->get_title();
                    ImGui::MenuItem(title.c_str(), nullptr, &_panel->open());
                }

                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }
    }
} // namespace sgm