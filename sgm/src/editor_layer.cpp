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
#include "panels/panels.h"
#include <sge/scene/scene_serializer.h>
#include <sge/imgui/imgui_layer.h>
namespace sgm {
    void editor_layer::on_attach() {
        fs::path scene_path = project::get().get_start_scene();
        if (fs::exists(scene_path)) {
            editor_scene::load(scene_path);
            m_scene_path = scene_path;
        } else {
            spdlog::warn("attempted to load nonexistent start scene: {0}", scene_path.string());
        }

        add_panel<renderer_info_panel>();
        add_panel<viewport_panel>([this](const fs::path& path) { m_scene_path = path; });
        add_panel<scene_hierarchy_panel>();
        add_panel<editor_panel>();
        add_panel<content_browser_panel>();

        register_popups();
    }

    void editor_layer::on_update(timestep ts) {
        for (auto& _panel : m_panels) {
            _panel->update(ts);
        }

        editor_scene::on_update(ts);
    }

    void editor_layer::on_event(event& e) {
        event_dispatcher dispatcher(e);
        dispatcher.dispatch<key_pressed_event>(SGE_BIND_EVENT_FUNC(editor_layer::on_key));

        if (!e.handled) {
            editor_scene::on_event(e);
        }
    }

    static bool demo_window = false;
    void editor_layer::on_imgui_render() {
        static constexpr ImGuiConfigFlags required_flags =
            ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;

        ImGuiIO& io = ImGui::GetIO();
        if ((io.ConfigFlags & required_flags) != required_flags) {
            throw std::runtime_error("SGM cannot be run in this environment.");
        }

        texture_cache::new_frame();
        m_popup_manager.update();
        update_dockspace();

        if (demo_window) {
            ImGui::ShowDemoWindow(&demo_window);
        }

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

    bool editor_layer::on_key(key_pressed_event& e) {
        if (e.get_repeat_count() > 0) {
            return false;
        }

        bool control =
            input::get_key(key_code::LEFT_CONTROL) || input::get_key(key_code::RIGHT_CONTROL);
        bool shift = input::get_key(key_code::LEFT_SHIFT) || input::get_key(key_code::RIGHT_SHIFT);

        switch (e.get_key()) {
        case key_code::N:
            if (control) {
                new_scene();
                return true;
            }

            break;
        case key_code::O:
            if (control) {
                open();
                return true;
            }

            break;
        case key_code::S:
            if (control) {
                if (shift) {
                    save_as();
                } else {
                    save();
                }

                return true;
            }

            break;
        case key_code::R:
            if (control) {
                auto _scene = editor_scene::get_scene();
                project::reload_assembly({ _scene });
            }

            break;
        case key_code::Q:
            if (control) {
                application::get().quit();
                return true;
            }

            break;
#ifdef SGE_DEBUG
        case key_code::T:
            if (control && shift) {
                m_popup_manager.open("Theme picker");
                return true;
            }

            break;
        case key_code::D:
            if (control && shift) {
                demo_window = !demo_window;
                return true;
            }

            break;
#endif
        }

        return false;
    }

    void editor_layer::register_popups() {
        ImGuiStyle& style = ImGui::GetStyle();

        // about
        {
            static constexpr float about_popup_width = 800.f;

            std::function<void()> callback = [&]() {
                ImGui::TextWrapped(
                    "Simple Game Engine is an open source 2D game engine focused on easy and "
                    "streamlined development of video games.");

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.f, 0.f, 1.f));
                ImGui::Text("Please report issues to "
                            "https://github.com/yodasoda1219/sge/issues");
                ImGui::PopStyleColor();

                if (ImGui::Button("Close")) {
                    ImGui::CloseCurrentPopup();
                }

                static const std::string version_string =
                    "SGE v" + application::get_engine_version();
                float version_text_width = ImGui::CalcTextSize(version_string.c_str()).x;

                ImGui::SameLine(about_popup_width -
                                (style.FramePadding.x * 2.f + version_text_width));
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.f), "%s", version_string.c_str());
            };

            popup_manager::popup_data data;
            data.size.x = 600.f;
            data.callback = callback;

            m_popup_manager.register_popup("About", data);
        }

        // theme picker
        {
            popup_manager::popup_data data;
            data.modal = false;

            data.callback = []() {
                ImGuiStyle& style = ImGui::GetStyle();

                static std::vector<const char*> combo_data;
                if (combo_data.empty()) {
                    for (ImGuiCol color = 0; color < ImGuiCol_COUNT; color++) {
                        const char* name = ImGui::GetStyleColorName(color);
                        combo_data.push_back(name);
                    }
                }

                static ImGuiCol current_color = 0;
                ImGui::Combo("ID", &current_color, combo_data.data(), combo_data.size());
                ImGui::ColorPicker4("##color-edit", &style.Colors[current_color].x);

                if (ImGui::Button("Export")) {
                    fs::path export_path = fs::current_path() / "bin" / "style.txt";
                    fs::path directory = export_path.parent_path();

                    if (!fs::exists(directory)) {
                        fs::create_directories(directory);
                    }

                    std::ofstream stream(export_path);
                    stream << "ImGuiStyle& style = ImGui::GetStyle();" << std::endl;

                    for (ImGuiCol id = 0; id < ImGuiCol_COUNT; id++) {
                        ImVec4 color = style.Colors[id];

                        char buffer[512];
                        sprintf(buffer, "style.Colors[%s] = ImVec4(%ff, %ff, %ff, %ff);",
                                ImGui::GetStyleColorName(id), color.x, color.y, color.z, color.w);

                        stream << buffer << std::endl;
                    }

                    stream.close();
                }

                ImGui::SameLine();
                if (ImGui::Button("Close")) {
                    ImGui::CloseCurrentPopup();
                }
            };

            m_popup_manager.register_popup("Theme picker", data);
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

        ImVec4* colors = ImGui::GetStyle().Colors;
        ImGui::PushStyleColor(ImGuiCol_ChildBg, colors[ImGuiCol_MenuBarBg]);

        ImGuiID window_id = ImGui::GetID("toolbar");
        ImGui::BeginChild(window_id, ImVec2(0.f, toolbar_height), false,
                          ImGuiWindowFlags_NoScrollbar);

        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);

        float cursor_pos = (ImGui::GetWindowContentRegionMax().x - icon_size) / 2.f;
        ImGui::SetCursorPosX(cursor_pos);

        bool running = editor_scene::running();
        std::string icon_name = running ? "stop" : "play";
        ref<texture_2d> icon = icon_directory::get(icon_name);

        ImVec4 color = colors[ImGuiCol_ButtonHovered];
        color.w = 0.5f;
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);

        color = colors[ImGuiCol_ButtonActive];
        color.w = 0.5f;
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
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
                if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
                    new_scene();
                }

                if (ImGui::MenuItem("Open...", "Ctrl+O")) {
                    open();
                }

                if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
                    save_as();
                }

                if (ImGui::MenuItem("Save", "Ctrl+S")) {
                    save();
                }

                ImGui::Separator();
                if (ImGui::MenuItem("Reload C# assembly", "Ctrl+R")) {
                    auto _scene = editor_scene::get_scene();
                    project::reload_assembly({ _scene });
                }

                ImGui::Separator();
                if (ImGui::MenuItem("Quit", "Ctrl+Q")) {
                    application::get().quit();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                for (auto& _panel : m_panels) {
                    std::string title = _panel->get_title();
                    ImGui::MenuItem(title.c_str(), nullptr, &_panel->open());
                }

#ifdef SGE_DEBUG
                ImGui::Separator();
                ImGui::MenuItem("Demo window", "Ctrl+Shift+D", &demo_window);

                if (ImGui::MenuItem("Theme picker", "Ctrl+Shift+T")) {
                    m_popup_manager.open("Theme picker");
                }
#endif

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("About")) {
                    m_popup_manager.open("About");
                }

                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }
    }

    void editor_layer::new_scene() {
        // todo(nora): if edited, confirm load

        if (editor_scene::running()) {
            editor_scene::stop();
        }

        auto _scene = editor_scene::get_scene();
        _scene->clear();

        m_scene_path.reset();
        garbage_collector::collect();
    }

    static const dialog_file_filter scene_filter = { "SGE scene (*.sgescene)", "*.sgescene" };

    void editor_layer::open() {
        // todo(nora): if edited, confirm load

        auto _window = application::get().get_window();
        auto path = _window->file_dialog(dialog_mode::open, { scene_filter });

        if (path.has_value()) {
            editor_scene::load(path.value());
            m_scene_path = path;
        }
    }

    void editor_layer::save_as() {
        auto _window = application::get().get_window();
        auto path = _window->file_dialog(dialog_mode::save, { scene_filter });

        if (path.has_value()) {
            editor_scene::save(path.value());
            m_scene_path = path;
        }
    }

    void editor_layer::save() {
        if (m_scene_path.has_value()) {
            editor_scene::save(m_scene_path.value());
        } else {
            save_as();
        }
    }
} // namespace sgm
