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
#include "editor_scene.h"

#include <sge/asset/asset_serializers.h>
#include <sge/renderer/renderer.h>
#include <sge/renderer/framebuffer.h>

// i absolutely DESPISE regular expressions but here we are
#include <regex>

namespace sgm {
    static const std::string overwrite_prefab_popup_name = "Overwrite prefab";

    static bool dump_texture(ref<texture_2d> texture, fs::path asset_path) {
        auto data = texture->get_image()->dump();
        if (!data) {
            return false;
        }

        std::string path = asset_path.string();
        if (path.empty()) {
            return false;
        }

        size_t position;
        while ((position = path.find("..")) != std::string::npos) {
            path.replace(position, 2, "__");
        }

        fs::path dump_path = fs::current_path() / "assets" / "logs" / "dump" /
                             project::get().get_name() / "textures" / path;

        fs::path directory = dump_path.parent_path();
        fs::create_directories(directory);

        return data->write(dump_path);
    }

    static void dump_assets(asset_manager& manager) {
#ifdef SGE_DEBUG
        auto& registry = manager.registry;

        for (const auto& [current_path, desc] : registry) {
            ref<asset> _asset = manager.get_asset(current_path);
            if (_asset && _asset->get_asset_type() == asset_type::texture_2d) {
                auto texture = _asset.as<texture_2d>();
                if (!dump_texture(texture, current_path)) {
                    spdlog::warn("failed to dump texture: {0}", current_path.string());
                }
            }
        }
#endif
    }

    class browser_history {
    public:
        browser_history(const fs::path& root) {
            m_paths.push_back(root);
            m_current = m_paths.begin();
        }

        browser_history(const browser_history&) = delete;
        browser_history& operator=(const browser_history&) = delete;

        bool can_undo() const { return m_current != m_paths.begin(); }

        bool can_redo() const {
            auto temp = m_current;
            temp++;
            return temp != m_paths.end();
        }

        bool undo() {
            if (!can_undo()) {
                return false;
            }

            m_current--;
            return true;
        }

        bool redo() {
            if (!can_redo()) {
                return false;
            }

            m_current++;
            return true;
        }

        void push(const fs::path& path) {
            auto temp = m_current;
            temp++;

            while (temp != m_paths.end()) {
                m_paths.erase(temp);

                temp = m_current;
                temp++;
            }

            m_paths.insert(m_paths.end(), path);
            m_current++;

            static constexpr size_t max_paths = 10;
            while (m_paths.size() > max_paths) {
                temp = m_paths.begin();
                m_paths.erase(temp);
            }
        }

        const fs::path& get() const { return *m_current; }

    private:
        using container_t = std::list<fs::path>;

        container_t m_paths;
        container_t::const_iterator m_current;
    };

    content_browser_panel::content_browser_panel() {
        auto& _project = project::get();
        // dump_assets(_project.get_asset_manager());

        m_root = _project.get_asset_dir();
        m_history = new browser_history(m_root);
        m_padding = 16.f;
        m_icon_size = 128.f;

        build_extension_data();
        rebuild_directory_data();

        auto& app = application::get();
        m_remove_watcher = app.watch_directory(m_root);

        m_prefab_override_params = nullptr;
    }

    content_browser_panel::~content_browser_panel() {
        delete m_history;
        if (m_remove_watcher) {
            auto& app = application::get();
            app.remove_watched_directory(m_root);
        }
    }

    void content_browser_panel::update(timestep ts) {
        if (m_modified_files.empty()) {
            return;
        }

        renderer::wait();
        auto& manager = project::get().get_asset_manager();
        for (const auto& path : m_modified_files) {
            if (manager.is_asset_loaded(path)) {
                auto _asset = manager.get_asset(path);

                if (_asset->reload()) {
                    continue;
                }
            }

            manager.clear_cache_entry(path);
        }

        m_modified_files.clear();
    }

    void content_browser_panel::on_event(event& e) {
        event_dispatcher dispatcher(e);
        dispatcher.dispatch<file_changed_event>(
            SGE_BIND_EVENT_FUNC(content_browser_panel::on_file_changed));
    }

    void content_browser_panel::register_popups(popup_manager& popup_manager_) {
        {
            popup_manager::popup_data data;
            data.callback = [this]() mutable {
                if (m_prefab_override_params == nullptr) {
                    ImGui::CloseCurrentPopup();
                    return;
                }

                std::string string_path = m_prefab_override_params->path.string();
                ImGui::Text("Overwrite %s?", string_path.c_str());

                bool overwritten = false;
                if (ImGui::Button("Yes")) {
                    m_prefab_override_params->write_callback();
                    overwritten = true;
                }

                ImGui::SameLine();
                if (ImGui::Button("No") || overwritten) {
                    delete m_prefab_override_params;
                    m_prefab_override_params = nullptr;
                }
            };

            popup_manager_.register_popup(overwrite_prefab_popup_name, data);
        }

        m_popup_manager = &popup_manager_;
    }

    void content_browser_panel::render() {
        ImGuiStyle& style = ImGui::GetStyle();

        {
            auto arrow = icon_directory::get("arrow");

            static constexpr float button_size = 25.f;
            ImGui::BeginChild("navigation-bar",
                              ImVec2(0.f, button_size + (style.FramePadding.y * 2.f)));

            ImGui::PushID("undo");
            ImGui::BeginDisabled(!m_history->can_undo());
            if (ImGui::ImageButton(arrow->get_imgui_id(), ImVec2(button_size, button_size),
                                   ImVec2(1.f, 0.f), ImVec2(0.f, 1.f))) {
                m_history->undo();
            }

            ImGui::EndDisabled();
            ImGui::PopID();
            ImGui::SameLine();

            ImGui::PushID("redo");
            ImGui::BeginDisabled(!m_history->can_redo());
            if (ImGui::ImageButton(arrow->get_imgui_id(), ImVec2(button_size, button_size))) {
                m_history->redo();
            }

            ImGui::EndDisabled();
            ImGui::PopID();

            ImGui::EndChild();
            ImGui::Separator();
        }

        const auto& current_path = m_history->get();
        ImGui::BeginChild("directory-items");

        float cell_size = m_padding + m_icon_size;
        float panel_width = ImGui::GetContentRegionAvail().x;

        int32_t column_count = (int32_t)glm::floor(panel_width / cell_size);
        if (column_count < 1) {
            column_count = 1;
        }

        std::unordered_set<fs::path, path_hasher> to_delete;
        ImGui::Columns(column_count, nullptr, false);
        for (const auto& entry : fs::directory_iterator(current_path)) {
            fs::path path = fs::absolute(entry.path());
            fs::path asset_path = path.lexically_relative(m_root);

            bool irrelevant_asset = false;
            const auto& directory_data = get_directory_data(current_path);

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

            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Delete")) {
                    to_delete.insert(path);
                }

                ImGui::EndPopup();
            }

            ImGui::PopStyleColor();
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                if (entry.is_directory()) {
                    m_history->push(path);
                } else {
                    // maybe some kind of open function?
                }
            }

            // todo: fix filename display
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
        ImGui::EndChild();

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup("item-context");
        }

        if (ImGui::BeginPopup("item-context")) {
            // todo: add items

            ImGui::EndPopup();
        }

        for (const auto& path : to_delete) {
            fs::remove_all(path);
        }

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("entity")) {
                if (editor_scene::running()) {
                    spdlog::warn("attempted to create a prefab during runtime");
                } else {
                    guid id = *(guid*)payload->Data;
                    entity e = editor_scene::get_scene()->find_guid(id);

                    if (!e) {
                        throw std::runtime_error("invalid guid passed!");
                    }

                    std::string tag = e.get_component<tag_component>().tag;
                    if (tag.empty()) {
                        tag = "Entity";
                    }

                    std::regex re("[#%&\\{\\}\\\\<>\\*\\?/\\$\\!'\":@\\+`\\|=]");
                    std::cmatch match;

                    if (std::regex_search(tag.c_str(), match, re)) {
                        spdlog::warn("attempted to create prefab with illegal tag - replacing "
                                     "illegal characters");

                        for (auto& submatch : match) {
                            if (!submatch.matched) {
                                continue;
                            }

                            size_t position = tag.find(submatch.first);
                            size_t count = strlen(submatch.first);

                            std::string replacement;
                            for (size_t i = 0; i < count; i++) {
                                replacement += '-';
                            }

                            tag.replace(position, count, replacement);
                        }
                    }

                    fs::path path = current_path / (tag + ".sgeprefab");
                    auto write = [e, path, this]() mutable {
                        prefab::from_entity(e, path);
                        rebuild_directory_data();
                    };

                    if (!fs::exists(path)) {
                        write();
                    } else if (m_prefab_override_params == nullptr) {
                        m_prefab_override_params = new prefab_override_params_t;
                        m_prefab_override_params->write_callback = write;
                        m_prefab_override_params->path = path;

                        m_popup_manager->open(overwrite_prefab_popup_name);
                    } else {
                        spdlog::error("could not open popup!");
                    }
                }
            }

            ImGui::EndDragDropTarget();
        }
    }

    bool content_browser_panel::on_file_changed(file_changed_event& e) {
        if (e.get_watched_directory() != m_root) {
            return false;
        }

        const auto& path = e.get_path();
        fs::path asset_path = fs::relative(path, m_root);

        bool handled = false;
        bool tree_changed = false;

        auto& manager = project::get().get_asset_manager();
        auto& registry = manager.registry;

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
        case file_status::deleted:
            if (m_modified_files.find(asset_path) != m_modified_files.end()) {
                m_modified_files.erase(asset_path);
            }

            handled = true;
            tree_changed |= registry.remove_asset(asset_path);

            break;
        case file_status::modified:
            if (m_modified_files.find(asset_path) == m_modified_files.end()) {
                m_modified_files.insert(asset_path);
            }

            break;
        }

        if (tree_changed) {
            rebuild_directory_data();
        }

        // dump_assets(manager);
        return handled;
    }

    void content_browser_panel::build_extension_data() {
        if (!m_extension_data.empty()) {
            m_extension_data.clear();
        }

        auto add_extension_entry = [this](const fs::path& extension,
                                          const asset_extension_data_t& data) mutable {
            m_extension_data.insert(std::make_pair(extension, data));
        };

        // images
        {
            asset_extension_data_t data;
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
            asset_extension_data_t data;
            data.drag_drop_id = "scene";
            data.icon_name = "file"; // for now

            add_extension_entry(".sgescene", data);
        }

        // shaders
        {
            asset_extension_data_t data;
            data.drag_drop_id = "shader";
            data.icon_name = "file"; // for now
            data.type = asset_type::shader;

            static std::unordered_set<fs::path, path_hasher> shader_extensions = { ".hlsl",
                                                                                   ".glsl" };
            for (const fs::path& extension : shader_extensions) {
                add_extension_entry(extension, data);
            }
        }

        // scripts
        {
            asset_extension_data_t data;
            data.drag_drop_id = "script";
            data.icon_name = "file"; // for now

            add_extension_entry(".cs", data);
        }

        // prefabs
        {
            asset_extension_data_t data;
            data.drag_drop_id = "prefab";
            data.icon_name = "file"; // for now
            data.type = asset_type::prefab;

            add_extension_entry(".sgeprefab", data);
        }

        // sounds
        {
            asset_extension_data_t data;
            data.drag_drop_id = "sound";
            data.icon_name = "file"; // for now
            data.type = asset_type::sound;

            // https://github.com/mackron/miniaudio#major-features
            static std::unordered_set<fs::path, path_hasher> sound_extensions = { ".wav", ".flac",
                                                                                  ".mp3", ".ogg" };

            for (const fs::path& extension : sound_extensions) {
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

    void content_browser_panel::rebuild_directory_data() {
        m_subdirectories.clear();
        build_directory_data(m_root, m_root_data);
    }

    void content_browser_panel::build_directory_data(const fs::path& path,
                                                     asset_directory_data_t& data) {
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
                    asset_directory_data_t subdirectory;
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

    const content_browser_panel::asset_directory_data_t& content_browser_panel::get_directory_data(
        const fs::path& path) {
        auto asset_path = path.lexically_relative(m_root);

        asset_directory_data_t* current = &m_root_data;
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
