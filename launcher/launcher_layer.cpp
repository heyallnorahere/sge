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

#include "launcher_pch.h"
#include "launcher_layer.h"
#include <sge/imgui/imgui_layer.h>

namespace sgm::launcher {
    static const std::string sge_dir_env_var = "SGE_DIR";
    static const std::string sge_dir_popup_name = "Select SGE directory";

    static const std::vector<dialog_file_filter> project_filters = { { "SGE project (*.sgeproject)",
                                                                       "*.sgeproject" } };

    void launcher_layer::on_attach() {
        // sge dir selection
        {
            popup_manager::popup_data data;
            data.size.x = 800.f;

            data.callback = []() {
                ImGui::Text("Please select the directory in which SGE was installed.");

                static fs::path home_dir;
                if (home_dir.empty()) {
                    home_dir = environment::get_home_directory();
                    home_dir.make_preferred();
                }

                static std::string default_path = (home_dir / "src" / "sge").string();
                static std::string path;
                ImGui::InputTextWithHint("##sge-dir", default_path.c_str(), &path);

#ifndef SGE_PLATFORM_WINDOWS
                ImGui::TextWrapped("Note: in order for environmental changes to take effect, you "
                                   "may need to relaunch your terminal.");
#endif

                if (ImGui::Button("Confirm")) {
                    fs::path dir_path = path.empty() ? default_path : path;
                    dir_path = dir_path.lexically_normal();
                    dir_path.make_preferred();

                    if (fs::exists(dir_path)) {
                        if (fs::is_directory(dir_path)) {
                            if (environment::set(sge_dir_env_var, dir_path.string())) {
                                ImGui::CloseCurrentPopup();
                            } else {
                                spdlog::error("failed to set {0}!", sge_dir_env_var);
                            }
                        } else {
                            spdlog::error("path {0} is not a directory!", dir_path.string());
                        }
                    } else {
                        spdlog::error("path {0} does not exist!", dir_path.string());
                    }
                }
            };

            m_popup_manager.register_popup(sge_dir_popup_name, data);
        }

        // project creation
        {
            popup_manager::popup_data data;
            data.size.x = 800.f;

            data.callback = [this]() {
                static const std::string default_name = "My Project";
                static const std::string default_path = (environment::get_home_directory() / "src" /
                                                         "MyProject" / "MyProject.sgeproject")
                                                            .string();

                static std::string name;
                static std::string path;

                static const std::string name_label = "Project name";
                static const std::string path_label = "Project path";

                float name_size = ImGui::CalcTextSize(name_label.c_str()).x;
                float path_size = ImGui::CalcTextSize(path_label.c_str()).x;
                float offset = name_size < path_size ? path_size : name_size;

                ImGuiStyle& style = ImGui::GetStyle();
                float spacing = style.WindowPadding.x + style.ItemSpacing.x;

                ImGui::Text("%s", name_label.c_str());
                ImGui::SameLine(offset, spacing);
                ImGui::InputTextWithHint("##name", default_name.c_str(), &name);

                ImGui::Text("%s", path_label.c_str());
                ImGui::SameLine(offset, spacing);

                ImGui::InputTextWithHint("##path", default_path.c_str(), &path);
                ImGui::SameLine();

                if (ImGui::Button("...")) {
                    auto& app = application::get();
                    auto _window = app.get_window();

                    auto result = _window->file_dialog(dialog_mode::save, project_filters);
                    if (result.has_value()) {
                        path = result->string();
                    }
                }

                if (ImGui::Button("Create")) {
                    project_info info;
                    info.name = name.empty() ? default_name : name;
                    info.path = path.empty() ? default_path : path;

                    name.clear();
                    path.clear();

                    m_callbacks.create_project(info);
                }

                ImGui::SameLine();
                if (ImGui::Button("Cancel")) {
                    name.clear();
                    path.clear();

                    ImGui::CloseCurrentPopup();
                }
            };

            m_popup_manager.register_popup("Create project", data);
        }
    }

    void launcher_layer::on_imgui_render() {
        ImGuiStyle& style = ImGui::GetStyle();

        static constexpr ImGuiConfigFlags required_flags =
            ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;

        ImGuiIO& io = ImGui::GetIO();
        if ((io.ConfigFlags & required_flags) != required_flags) {
            throw std::runtime_error("The SGM launcher cannot be run in this environment.");
        }

        static constexpr ImGuiWindowFlags window_flags =
            ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);

        static constexpr float window_padding = 20.f;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(window_padding, window_padding));

        ImGui::Begin("Main launcher window", nullptr, window_flags);
        ImGui::PopStyleVar(3);

        if (!m_sge_dir_set) {
            if (!m_popup_manager.is_open(sge_dir_popup_name)) {
                if (environment::has(sge_dir_env_var)) {
                    m_sge_dir_set = true;
                } else {
                    m_popup_manager.open(sge_dir_popup_name);
                }
            }
        } else {
            static constexpr float padding = 20.f;
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(padding, window_padding));

            ImVec2 window_content_region = ImGui::GetContentRegionAvail();
            float column_size = window_content_region.x * 2.f / 5.f;

            ImGui::Columns(2, nullptr, false);
            ImGui::SetColumnWidth(0, window_content_region.x - column_size);
            ImGui::SetColumnWidth(1, column_size);

            // recent projects
            {
                static std::optional<size_t> hovered_project;

                size_t unhovered_projects = 0;
                for (size_t i = 0; i < 3; i++) {
                    std::string id = "project-" + std::to_string(i);
                    bool hovered = hovered_project == i;

                    ImGuiCol bg_color;
                    if (hovered) {
                        bg_color = ImGuiCol_ButtonHovered;
                    } else {
                        bg_color = ImGuiCol_Button;
                    }

                    ImGui::PushStyleColor(ImGuiCol_ChildBg, style.Colors[bg_color]);
                    ImGui::BeginChild(id.c_str(), ImVec2(0.f, 100.f), true);
                    ImGui::PopStyleColor();

                    ImGui::Text("Test project %u", (uint32_t)(i + 1));
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.f), "Some path");

                    ImGui::EndChild();
                    if (ImGui::IsItemHovered()) {
                        hovered_project = i;

                        if (ImGui::IsItemClicked()) {
                            fs::path sge_dir = environment::get(sge_dir_env_var);
                            m_callbacks.open_project(sge_dir / "sandbox-project" /
                                                     "sandbox.sgeproject");
                        }
                    } else {
                        unhovered_projects++;
                    }
                }

                if (unhovered_projects == 3) {
                    hovered_project.reset();
                }
            }

            // create/open project
            ImGui::NextColumn();
            {

                static std::optional<size_t> hovered_button;
                size_t unhovered_buttons = 0;
                size_t total_buttons = 0;

                auto& app = application::get();
                auto render_button = [&](ref<texture_2d> image, const std::string& text,
                                         const std::string& id) {
                    bool hovered = hovered_button == total_buttons;

                    ImGuiCol bg_color;
                    if (hovered) {
                        bg_color = ImGuiCol_ButtonHovered;
                    } else {
                        bg_color = ImGuiCol_Button;
                    }

                    static constexpr ImGuiWindowFlags button_flags =
                        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

                    ImGui::PushStyleColor(ImGuiCol_ChildBg, style.Colors[bg_color]);
                    ImGui::BeginChild(id.c_str(), ImVec2(0.f, 150.f), true, button_flags);
                    ImGui::PopStyleColor();

                    ImVec2 content_region = ImGui::GetContentRegionAvail();
                    float image_size = content_region.y;
                    ImGui::Image(image->get_imgui_id(), ImVec2(image_size, image_size));

                    ImFont* bold = app.get_imgui_layer().get_font("Roboto-Bold.ttf");
                    ImGui::PushFont(bold);

                    ImVec2 text_size = ImGui::CalcTextSize(text.c_str());
                    float text_region = content_region.x - (image_size + style.ItemSpacing.x);
                    float text_pos = (text_region - text_size.x) / 2.f;
                    ImGui::SameLine(text_pos + image_size, 0.f);

                    float cursor_pos = ImGui::GetCursorPosY();
                    cursor_pos += (content_region.y - text_size.y) / 2.f;
                    ImGui::SetCursorPosY(cursor_pos);

                    ImGui::Text("%s", text.c_str());
                    ImGui::PopFont();
                    ImGui::EndChild();

                    bool clicked = false;
                    if (ImGui::IsItemHovered()) {
                        hovered_button = total_buttons;

                        if (ImGui::IsItemClicked()) {
                            clicked = true;
                        }
                    } else {
                        unhovered_buttons++;
                    }

                    total_buttons++;
                    return clicked;
                };

                if (!m_test_texture) {
                    // todo: design actual icons for creating and opening projects
                    m_test_texture = texture_2d::load("assets/icons/play.png");
                }

                if (render_button(m_test_texture, "Create Project", "create-project")) {
                    m_popup_manager.open("Create project");
                }

                if (render_button(m_test_texture, "Open Project", "open-project")) {
                    auto _window = app.get_window();
                    auto selected = _window->file_dialog(dialog_mode::open, project_filters);

                    if (selected.has_value()) {
                        m_callbacks.open_project(selected.value());
                    }
                }

                if (unhovered_buttons == total_buttons) {
                    hovered_button.reset();
                }
            }

            ImGui::PopStyleVar(2);
            ImGui::Columns(1);
        }

        ImGui::End();
        m_popup_manager.update();
    }
} // namespace sgm::launcher
