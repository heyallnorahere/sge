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
#include <sge/imgui/popup_manager.h>

namespace sgm::launcher {
    class launcher_layer : public layer {
    public:
        struct recent_project_t {
            std::string name;
            fs::path path;

            size_t hash() const {
                size_t name_hash = std::hash<std::string>()(name);
                size_t path_hash = path_hasher()(path);

                return (path_hash << 1) ^ name_hash;
            }
        };

        struct project_info_t {
            fs::path path;
            std::string name;
        };

        struct app_callbacks {
            std::function<bool(const project_info_t&, std::string& error)> create_project;
            std::function<void(const fs::path& path)> open_project;
        };

        launcher_layer(const app_callbacks& callbacks) : layer("Launcher Layer") {
            m_callbacks = callbacks;
        }

        virtual void on_attach() override;
        virtual void on_imgui_render() override;

        void add_to_recent(const fs::path& path);

    private:
        void read_recent_projects();
        void write_recent_projects();

        app_callbacks m_callbacks;
        popup_manager m_popup_manager;

        std::list<recent_project_t> m_recent_projects;
        bool m_sge_dir_set = false;
        ref<texture_2d> m_test_texture;
    };
} // namespace sgm::launcher