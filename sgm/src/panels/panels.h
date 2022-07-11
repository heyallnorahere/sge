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

#pragma once
#include "panel.h"
#include <sge/renderer/texture.h>

namespace sgm {
    class renderer_info_panel : public panel {
    public:
        virtual void update(timestep ts) override;

        virtual void render() override;

        virtual std::string get_title() override { return "Renderer Info"; }
        virtual panel_id get_id() override { return panel_id::renderer_info; }

    private:
        bool m_reload_shaders = false;
    };

    class viewport_panel : public panel {
    public:
        viewport_panel(const std::function<void(const fs::path&)>& load_scene_callback);

        virtual void update(timestep ts) override;

        virtual void begin(const char* title, bool* open) override;
        virtual void render() override;

        virtual std::string get_title() override { return "Viewport"; }
        virtual panel_id get_id() override { return panel_id::viewport; }

    private:
        void verify_size();
        void invalidate_texture();

        ref<texture_2d> m_current_texture;
        std::optional<glm::uvec2> m_new_size;
        std::function<void(const fs::path&)> m_load_scene_callback;
    };

    class scene_hierarchy_panel : public panel {
    public:
        virtual void render() override;

        virtual std::string get_title() override { return "Scene Hierarchy"; }
        virtual panel_id get_id() override { return panel_id::scene_hierarchy; }
    };

    class editor_panel : public panel {
    public:
        editor_panel(const std::function<void(const std::string&)>& popup_callback);
        virtual ~editor_panel() override;

        virtual void register_popups(popup_manager& popup_manager_) override;
        virtual void render() override;

        virtual std::string get_title() override { return "Editor"; }
        virtual panel_id get_id() override { return panel_id::editor; }

        void clear_section_header_cache() { m_section_header_cache.clear(); }

    private:
        using script_control_t = std::function<void(void*, void*, const std::string&)>;
        enum class filter_editor_type { category, mask };

        struct filter_editor_data_t {
            filter_editor_type type;
            uint16_t* field;
        };

        struct section_header_t {
            std::string name;
            std::vector<void*> properties;
            std::vector<section_header_t> subheaders;
        };

        void cache_script_class(void* _class);
        void draw_property_controls();
        void render_header(void* script_object, const section_header_t& header);

        std::unordered_map<void*, std::vector<section_header_t>> m_section_header_cache;
        size_t m_callback_index;

        std::function<void(const std::string&)> m_popup_callback;
        popup_manager* m_popup_manager;
        filter_editor_data_t m_filter_editor_data;
    };

    class browser_history;
    class content_browser_panel : public panel {
    public:
        content_browser_panel();
        virtual ~content_browser_panel() override;

        virtual void update(timestep ts) override;
        virtual void on_event(event& e) override;

        virtual void register_popups(popup_manager& popup_manager_) override;
        virtual void render() override;

        virtual std::string get_title() override { return "Content Browser"; }
        virtual panel_id get_id() override { return panel_id::content_browser; }

    private:
        struct asset_extension_data_t {
            std::string drag_drop_id;
            std::string icon_name;
            std::optional<asset_type> type;
        };

        struct asset_directory_data_t {
            std::unordered_set<fs::path, path_hasher> files;
            std::unordered_map<fs::path, size_t, path_hasher> directories;
        };

        struct prefab_override_params_t {
            std::function<void()> write_callback;
            fs::path path;
        };

        bool on_file_changed(file_changed_event& e);

        void build_extension_data();
        ref<texture_2d> get_icon(const fs::path& path);
        std::string get_drag_drop_id(const fs::path& path);

        void rebuild_directory_data();
        void build_directory_data(const fs::path& path, asset_directory_data_t& data);
        const asset_directory_data_t& get_directory_data(const fs::path& path);

        std::unordered_set<fs::path, path_hasher> m_modified_files;

        fs::path m_root;
        browser_history* m_history;
        float m_padding, m_icon_size;
        std::unordered_map<fs::path, asset_extension_data_t, path_hasher> m_extension_data;

        asset_directory_data_t m_root_data;
        std::vector<asset_directory_data_t> m_subdirectories;
        bool m_remove_watcher;

        popup_manager* m_popup_manager;
        prefab_override_params_t* m_prefab_override_params;
    };
} // namespace sgm
