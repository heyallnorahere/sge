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

#pragma onces
#include <sge/imgui/popup_manager.h>

namespace sgm::launcher {
    class launcher_layer : public layer {
    public:
        struct project_info {
            fs::path directory;
            std::string name;
        };

        struct app_callbacks {
            std::function<void(const project_info&)> create_project;
            std::function<void(const fs::path& path)> open_project;
        };

        launcher_layer(const app_callbacks& callbacks) : layer("Launcher Layer") {
            m_callbacks = callbacks;
        }

        virtual void on_attach() override;
        virtual void on_imgui_render() override;

    private:
        app_callbacks m_callbacks;
        popup_manager m_popup_manager;

        bool m_work_dir_set = false;
        ref<texture_2d> m_test_texture;
    };
} // namespace sgm::launcher