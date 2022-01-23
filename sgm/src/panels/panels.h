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
    };

    class scene_hierarchy_panel : public panel {
    public:
        virtual void render() override;

        virtual std::string get_title() override { return "Scene Hierarchy"; }
        virtual panel_id get_id() override { return panel_id::scene_hierarchy; }
    };

    class editor_panel : public panel {
    public:
        editor_panel();

        virtual void render() override;

        virtual std::string get_title() override { return "Editor"; }
        virtual panel_id get_id() override { return panel_id::editor; }

    private:
        void draw_property_controls();

        using script_control_t = std::function<void(void*, void*, const std::string&)>;

        std::unordered_map<void*, script_control_t> m_script_controls;
    };

    class content_browser_panel : public panel {
    public:
        content_browser_panel();

        virtual void render() override;

        virtual std::string get_title() override { return "Content Browser"; }
        virtual panel_id get_id() override { return panel_id::content_browser; }

    private:
        struct asset_extension_data {
            std::string drag_drop_id;
            std::string icon_name;
            std::optional<asset_type> type;
        };

        void build_extension_data();
        ref<texture_2d> get_icon(const fs::path& path);
        std::string get_drag_drop_id(const fs::path& path);

        fs::path m_root, m_current;
        float m_padding, m_icon_size;
        std::unordered_map<fs::path, asset_extension_data> m_extension_data;
    };
} // namespace sgm
