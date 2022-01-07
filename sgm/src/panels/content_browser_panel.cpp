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
#include "icon_directory.h"
namespace sgm {
    content_browser_panel::content_browser_panel() {
        // todo: take from project
        m_current = m_root = fs::current_path() / "assets";
        rebuild_breadcrumb_data();

        m_padding = 16.f;
        m_icon_size = 128.f;
    }

    void content_browser_panel::render() {
        if (m_current != m_root) {
            if (ImGui::Button("Back")) {
                m_current = m_current.parent_path();
                rebuild_breadcrumb_data();
            }

            ImGui::Separator();
        }

        float cell_size = m_padding + m_icon_size;
        float panel_width = ImGui::GetContentRegionAvail().x;

        int32_t column_count = (int32_t)glm::floor(panel_width / cell_size);
        if (column_count < 1) {
            column_count = 1;
        }
        ImGui::Columns(column_count, nullptr, false);

        for (const auto& entry : fs::directory_iterator(m_current)) {
            auto path = fs::absolute(entry.path());

            std::string filename = path.filename().string();
            ImGui::PushID(filename.c_str());

            // todo: different icons per file type
            std::string icon_name = entry.is_directory() ? "directory" : "file";
            auto icon = icon_directory::get(icon_name);

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
            ImGui::ImageButton(icon->get_imgui_id(), ImVec2(m_icon_size, m_icon_size));

            if (ImGui::BeginDragDropSource()) {
                std::string path_data = path.string();
                const char* c_str = path_data.c_str();

                ImGui::Text(filename.c_str());
                ImGui::SetDragDropPayload("content-browser-file", c_str,
                                          (path_data.length() + 1) * sizeof(char));

                ImGui::EndDragDropSource();
            }

            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                if (entry.is_directory()) {
                    m_current = path;
                    rebuild_breadcrumb_data();
                } else {
                    // maybe some kind of open function?
                }
            }

            {
                float text_width = ImGui::CalcTextSize(filename.c_str()).x;

                float indentation = (m_icon_size - text_width) / 2.f;
                if (indentation < 0.f) {
                    indentation = 0.f;
                }

                ImGui::Indent(indentation);
                ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + (m_icon_size - indentation));

                ImGui::TextWrapped(filename.c_str());

                ImGui::PopTextWrapPos();
                ImGui::Unindent(indentation);
            }

            ImGui::PopID();
            ImGui::NextColumn();
        }

        ImGui::Columns(1);
    }

    void content_browser_panel::rebuild_breadcrumb_data() {
        m_breadcrumb_data.clear();

        fs::path current = m_current;
        while (current.parent_path() != current.root_path()) {
            if (current.has_filename()) {
                auto filename = current.filename();
                m_breadcrumb_data.push_back(filename.string());
            }

            current = current.parent_path();
        }
    }
} // namespace sgm
