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
    content_browser_panel::content_browser_panel() {
        build_extension_data();

        m_current = m_root = project::get().get_asset_dir();
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
                fs::path asset_path = path.lexically_relative(m_root);
                std::string payload_path = asset_path.string();
                const char* c_str = payload_path.c_str();

                std::string drag_drop_id = get_drag_drop_id(path);
                ImGui::SetDragDropPayload(drag_drop_id.c_str(), c_str,
                                          (payload_path.length() + 1) * sizeof(char));

                ImGui::Text("%s", filename.c_str());
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

                ImGui::TextWrapped("%s", filename.c_str());

                ImGui::PopTextWrapPos();
                ImGui::Unindent(indentation);
            }

            ImGui::PopID();
            ImGui::NextColumn();
        }

        ImGui::Columns(1);
    }

    void content_browser_panel::build_extension_data() {
        if (!m_extension_data.empty()) {
            m_extension_data.clear();
        }

        auto add_extension_entry = [this](const fs::path& extension,
                                          const asset_extension_data& data) mutable {
            m_extension_data.insert(std::make_pair(extension, data));
        };

        // images
        {
            asset_extension_data data;
            data.drag_drop_id = "texture_2d";
            data.icon_name = "image";
            data.type = asset_type::texture_2d;

            static std::unordered_set<fs::path> image_extensions = { ".png", ".jpg", ".jpeg" };
            for (fs::path extension : image_extensions) {
                add_extension_entry(extension, data);
            }
        }

        // scenes
        {
            asset_extension_data data;
            data.drag_drop_id = "scene";
            data.icon_name = "file"; // for now
            add_extension_entry(".sgescene", data);
        }

        // shaders
        {
            asset_extension_data data;
            data.drag_drop_id = "shader";
            data.icon_name = "file"; // for now

            static std::unordered_set<fs::path> shader_extensions = { ".hlsl", ".glsl" };
            for (fs::path extension : shader_extensions) {
                add_extension_entry(extension, data);
            }
        }
    }

    ref<texture_2d> content_browser_panel::get_icon(const fs::path& path) {
        std::string icon_name = "file";
        if (fs::is_directory(path)) {
            icon_name = "directory";
        } else {
            if (path.has_extension()) {
                fs::path extension = path.extension();
                if (m_extension_data.find(extension) != m_extension_data.end()) {
                    const auto& data = m_extension_data[extension];
                    if (data.icon_name == "image") {
                        auto asset_path = path.lexically_relative(m_root);
                        auto _asset = project::get().get_asset_manager().get_asset(asset_path);

                        if (_asset) {
                            return _asset.as<texture_2d>();
                        }
                    } else {
                        icon_name = data.icon_name;
                    }
                }
            }
        }

        return icon_directory::get(icon_name);
    }

    std::string content_browser_panel::get_drag_drop_id(const fs::path& path) {
        if (fs::is_directory(path)) {
            return "directory";
        } else {
            if (path.has_extension()) {
                fs::path extension = path.extension();
                if (m_extension_data.find(extension) != m_extension_data.end()) {
                    return m_extension_data[extension].drag_drop_id;
                }
            }
        }

        return "file";
    }
} // namespace sgm
