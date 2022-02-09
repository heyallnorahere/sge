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

namespace sgm::launcher {
    static const std::string sge_dir_env_var = "SGE_DIR";
    static const std::string sge_dir_popup_name = "Select SGE directory";

    void launcher_layer::on_attach() {
        // sge dir selection
        {
            popup_manager::popup_data data;
            data.size.x = 800.f;

            data.callback = []() {
                ImGui::Text("Please select the directory in which SGE was installed.");

                static fs::path home_dir;
                if (home_dir.empty()) {
#ifdef SGE_PLATFORM_WINDOWS
                    std::string env_name = "USERPROFILE";
#else
                    std::string env_name = "HOME";
#endif

                    home_dir = environment::get(env_name);
                    home_dir = home_dir.make_preferred();
                }

                static std::string default_path = (home_dir / "src" / "sge").string();
                static std::string path;

                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.f));
                ImGui::InputTextWithHint("##sge-dir", default_path.c_str(), &path);
                ImGui::PopStyleColor();

                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.f),
                                   "Note: on MacOS and Linux, you may need to relaunch your "
                                   "terminal.");

                if (ImGui::Button("Confirm")) {
                    fs::path dir_path = path.empty() ? default_path : path;
                    dir_path = dir_path.lexically_normal();

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
    }

    void launcher_layer::on_imgui_render() {
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
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 1.f));
        ImGui::Begin("Main launcher window", nullptr, window_flags);

        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);

        if (!m_work_dir_set && !m_popup_manager.is_open(sge_dir_popup_name)) {
            if (environment::has(sge_dir_env_var)) {
                fs::path sge_dir = environment::get(sge_dir_env_var);
                fs::current_path(sge_dir);
            } else {
                m_popup_manager.open(sge_dir_popup_name);
            }
        }

        ImGui::End();
        m_popup_manager.update();
    }
} // namespace sgm::launcher