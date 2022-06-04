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
#include <sge/asset/asset_serializers.h>

namespace sgm {
    content_browser_panel::content_browser_panel() {
        m_current = m_root = project::get().get_asset_dir();
        m_padding = 16.f;
        m_icon_size = 128.f;

        build_extension_data();
        build_directory_data(m_root, m_root_data);

        auto& app = application::get();
        m_remove_watcher = app.watch_directory(m_root);
    }

    content_browser_panel::~content_browser_panel() {
        if (m_remove_watcher) {
            auto& app = application::get();
            app.remove_watched_directory(m_root);
        }
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
            fs::path path = fs::absolute(entry.path());
            fs::path asset_path = path.lexically_relative(m_root);

            bool irrelevant_asset = false;
            const auto& directory_data = get_directory_data(m_current);

            fs::path filename = path.filename();
            if (entry.is_directory()) {
                irrelevant_asset =
                    (directory_data.directories.find(filename) == directory_data.directories.end());
            } else {
                irrelevant_asset =
                    (directory_data.files.find(filename) == directory_data.files.end());
            }

            if (irrelevant_asset) {
                continue;
            }

            std::string filename_string = filename.string();
            ImGui::PushID(filename_string.c_str());
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));

            auto icon = get_icon(path);
            ImGui::ImageButton(icon->get_imgui_id(), ImVec2(m_icon_size, m_icon_size));

            if (ImGui::BeginDragDropSource()) {
                std::string payload_path = asset_path.string();
                const char* c_str = payload_path.c_str();

                std::string drag_drop_id = get_drag_drop_id(path);
                ImGui::SetDragDropPayload(drag_drop_id.c_str(), c_str,
                                          (payload_path.length() + 1) * sizeof(char));

                ImGui::Text("%s", filename_string.c_str());
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
                float text_width = ImGui::CalcTextSize(filename_string.c_str()).x;

                float indentation = (m_icon_size - text_width) / 2.f;
                if (indentation < 0.f) {
                    indentation = 0.f;
                }

                ImGui::Indent(indentation);
                ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + (m_icon_size - indentation));

                ImGui::TextWrapped("%s", filename_string.c_str());

                ImGui::PopTextWrapPos();
                ImGui::Unindent(indentation);
            }

            ImGui::PopID();
            ImGui::NextColumn();
        }

        ImGui::Columns(1);
    }

    void content_browser_panel::on_event(event& e) {
        event_dispatcher dispatcher(e);
        dispatcher.dispatch<file_changed_event>(
            SGE_BIND_EVENT_FUNC(content_browser_panel::on_file_changed));
    }

    bool content_browser_panel::on_file_changed(file_changed_event& e) {
        if (e.get_watched_directory() != m_root) {
            return false;
        }

        const auto& path = e.get_path();
        auto& registry = project::get().get_asset_manager().registry;

        bool handled = false;
        bool tree_changed = false;

        switch (e.get_status()) {
        case file_status::created: {
            std::optional<asset_type> type;
            if (path.has_extension()) {
                fs::path extension = path.extension();

                if (m_extension_data.find(extension) != m_extension_data.end()) {
                    type = m_extension_data.at(extension).type;
                }
            }

            if (type.has_value()) {
                asset_desc desc;
                desc.id = guid();
                desc.path = path;
                desc.type = type;

                ref<asset> loaded;
                if (asset_serializer::deserialize(desc, loaded)) {
                    tree_changed |= registry.register_asset(loaded);
                }
            } else {
                tree_changed |= registry.register_asset(path);
            }

        } break;
        case file_status::deleted: {
            fs::path asset_path = fs::relative(path, m_root);

            handled = true;
            tree_changed |= registry.remove_asset(asset_path);
        } break;
        }

        if (tree_changed) {
            build_directory_data(m_root, m_root_data);
        }

        return handled;
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

            static std::unordered_set<fs::path, path_hasher> image_extensions = { ".png", ".jpg",
                                                                                  ".jpeg" };
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

            static std::unordered_set<fs::path, path_hasher> shader_extensions = { ".hlsl",
                                                                                   ".glsl" };
            for (fs::path extension : shader_extensions) {
                add_extension_entry(extension, data);
            }
        }

        // scripts
        {
            asset_extension_data data;
            data.drag_drop_id = "script";
            data.icon_name = "file"; // for now

            add_extension_entry(".cs", data);
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

    void content_browser_panel::build_directory_data(const fs::path& path,
                                                     asset_directory_data& data) {
        data.files.clear();
        data.directories.clear();

        auto& registry = project::get().get_asset_manager().registry;
        for (const auto& entry : fs::directory_iterator(path)) {
            auto entry_path = fs::absolute(entry.path());
            auto filename = entry_path.filename();

            if (entry.is_directory()) {
                bool found = false;

                for (const auto& [asset_path, desc] : registry) {
                    std::string dir_string = entry_path.string();
                    std::string path_string = (m_root / asset_path).string();

                    if (path_string.substr(0, dir_string.length()) == dir_string) {
                        found = true;
                        break;
                    }
                }

                if (found) {
                    asset_directory_data subdirectory;
                    build_directory_data(entry_path, subdirectory);

                    data.directories.insert(std::make_pair(filename, m_subdirectories.size()));
                    m_subdirectories.push_back(subdirectory);
                }
            } else {
                fs::path asset_path = entry_path.lexically_relative(m_root);
                if (registry.contains(asset_path)) {
                    data.files.insert(filename);
                }
            }
        }
    }

    const content_browser_panel::asset_directory_data& content_browser_panel::get_directory_data(
        const fs::path& path) {
        auto asset_path = path.lexically_relative(m_root);

        asset_directory_data* current = &m_root_data;
        for (const auto& segment : asset_path) {
            if (segment == ".") {
                continue;
            }

            if (current->directories.find(segment) != current->directories.end()) {
                size_t index = current->directories[segment];
                current = &m_subdirectories[index];
            } else {
                spdlog::warn("directory {0} does not exist!", segment.string());
                break;
            }
        }

        return *current;
    }
} // namespace sgm
