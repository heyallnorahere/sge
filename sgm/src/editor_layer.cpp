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
#include "icon_directory.h"
#include <sge/scene/scene_serializer.h>
namespace sgm {
    void editor_layer::on_update(timestep ts) {
        for (auto& _panel : m_panels) {
            _panel->update(ts);
        }

        editor_scene::on_update(ts);
    }

    void editor_layer::on_event(event& e) {
        event_dispatcher dispatcher(e);

        // todo: on_key event for keyboard shortcuts

        if (!e.handled) {
            editor_scene::on_event(e);
        }
    }

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

        ImGui::Begin("Main SGM window", nullptr, window_flags);
        ImGui::PopStyleVar(3);

        update_menu_bar();
        update_toolbar();

        ImGui::DockSpace(ImGui::GetID("sgm-dockspace"));
        ImGui::End();
    }

    void editor_layer::update_toolbar() {
        static constexpr float toolbar_height = 35.f;
        static constexpr float padding = 2.f;
        static constexpr float icon_size = toolbar_height - padding * 2.f;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, padding));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0.f, 0.f));

        ImGuiID window_id = ImGui::GetID("toolbar");
        ImGui::BeginChild(window_id, ImVec2(0.f, toolbar_height), false, ImGuiWindowFlags_NoScrollbar);

        ImGui::PopStyleVar(2);

        float cursor_pos = (ImGui::GetWindowContentRegionMax().x - icon_size) / 2.f;
        ImGui::SetCursorPosX(cursor_pos);

        bool running = editor_scene::running();
        std::string icon_name = running ? "stop" : "play";
        ref<texture_2d> icon = icon_directory::get(icon_name);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
        ImVec4* colors = ImGui::GetStyle().Colors;

        ImVec4 color = colors[ImGuiCol_ButtonHovered];
        color.w = 0.5f;
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);

        color = colors[ImGuiCol_ButtonActive];
        color.w = 0.5f;
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);

        if (ImGui::ImageButton(icon->get_imgui_id(), ImVec2(icon_size, icon_size))) {
            if (running) {
                editor_scene::stop();
            } else {
                editor_scene::play();
            }
        }

        ImGui::PopStyleColor(3);
        ImGui::EndChild();
    }

    void editor_layer::update_menu_bar() {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                auto& app = application::get();
                {
                    auto _window = app.get_window();
                    static const std::vector<dialog_file_filter> filters = {
                        { "SGE scene (*.sgescene)", "*.sgescene" }
                    };

                    if (ImGui::MenuItem("Open...", "Ctrl+O")) {
                        auto path = _window->file_dialog(dialog_mode::open, filters);
                        if (path.has_value()) {
                            editor_scene::load(path.value());
                        }
                    }

                    if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
                        auto path = _window->file_dialog(dialog_mode::save, filters);
                        if (path.has_value()) {
                            editor_scene::save(path.value());
                        }
                    }
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Quit", "Ctrl+Q")) {
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
