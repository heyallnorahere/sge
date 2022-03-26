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
namespace sgm {
    class editor_layer : public layer {
    public:
        editor_layer() : layer("Editor Layer") {}

        virtual void on_attach() override;
        virtual void on_update(timestep ts) override;
        virtual void on_event(event& e) override;
        virtual void on_imgui_render() override;

    private:
        template <typename T, typename... Args>
        void add_panel(Args&&... args) {
            static_assert(std::is_base_of_v<panel, T>, "T must be derived from panel!");

            panel* instance = (panel*)new T(std::forward<Args>(args)...);
            m_panels.push_back(std::unique_ptr<panel>(instance));
        }

        bool on_key(key_pressed_event& e);

        void register_popups();

        void update_dockspace();
        void update_toolbar();
        void update_menu_bar();

        void new_scene();
        void open();
        void save_as();
        void save();

        std::vector<std::unique_ptr<panel>> m_panels;
        std::optional<fs::path> m_scene_path;
        popup_manager m_popup_manager;
    };
} // namespace sgm