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
#include "texture_cache.h"
namespace sgm {
    static std::unordered_set<std::string> image_extensions = { ".png", ".jpg", ".jpeg" };

    content_browser_panel::content_browser_panel() {
        // todo: take from project
        m_current = m_root = fs::absolute(fs::current_path() / "assets");
        m_padding = 16.f;
        m_icon_size = 128.f;
    }

    void content_browser_panel::render() {
        if (m_current != m_root) {
            if (ImGui::Button("Back")) {
                m_current = m_current.parent_path();
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
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));

            auto icon = get_icon(path);
            ImGui::ImageButton(icon->get_imgui_id(), ImVec2(m_icon_size, m_icon_size));

            if (ImGui::BeginDragDropSource()) {
                std::string payload_path = fs::relative(path).string();
                const char* c_str = payload_path.c_str();

                ImGui::Text(filename.c_str());
                ImGui::SetDragDropPayload("content-browser-file", c_str,
                                          (payload_path.length() + 1) * sizeof(char));

                ImGui::EndDragDropSource();
            }

            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                if (entry.is_directory()) {
                    m_current = path;
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

    ref<texture_2d> content_browser_panel::get_icon(const fs::path& path) {
        std::string extension = path.extension().string();
        if (image_extensions.find(extension) != image_extensions.end()) {
            std::string string_path = path.string();
            if (m_image_icon_blacklist.find(string_path) == m_image_icon_blacklist.end()) {
                ref<texture_2d> icon;

                if (m_file_textures.find(string_path) != m_file_textures.end()) {
                    icon = m_file_textures[string_path];
                } else {
                    // todo: reload when image changes
                    auto img_data = image_data::load(path);
                    if (img_data) {
                        texture_spec spec;
                        spec.filter = texture_filter::linear;
                        spec.wrap = texture_wrap::repeat;
                        spec.image = image_2d::create(img_data, image_usage_none);

                        icon = texture_2d::create(spec);
                        m_file_textures.insert(std::make_pair(string_path, icon));
                    } else {
                        spdlog::warn("could not load image at path {0} - adding to blacklist",
                                     string_path);
                        m_image_icon_blacklist.insert(string_path);
                    }
                }

                if (icon) {
                    texture_cache::add_texture(icon);
                    return icon;
                }
            }
        }

        // todo: different icons per file type
        std::string icon_name = fs::is_directory(path) ? "directory" : "file";
        return icon_directory::get(icon_name);
    }
} // namespace sgm
