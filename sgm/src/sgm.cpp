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
#include <sge/core/main.h>
#include "editor_layer.h"
#include "editor_scene.h"
#include "icon_directory.h"
#include "texture_cache.h"
namespace sgm {
    static const std::string sgm_title = "Simple Game Maker v" + application::get_engine_version();

    class sgm_app : public application {
    public:
        sgm_app() : application("SGM") {}
    
    protected:
        virtual void on_init() override {
            // defaults to sandbox project
            fs::path project_path = "sandbox-project/sandbox.sgeproject";

            std::vector<std::string> args;
            get_application_args(args);

            if (args.size() >= 2) {
                project_path = args[1];
            }

            project::load(fs::absolute(project_path));
            
            project& _project = project::get();
            m_window->set_title(sgm_title + " - " + _project.get_name());

            icon_directory::load();
            editor_scene::create();
            texture_cache::init();

            m_editor_layer = new editor_layer;
            push_layer(m_editor_layer);
        }

        virtual void on_shutdown() override {
            pop_layer(m_editor_layer);
            delete m_editor_layer;

            texture_cache::shutdown();
            editor_scene::destroy();
            icon_directory::clear();
        }

        virtual std::string get_window_title() override { return sgm_title; }
        virtual bool is_editor() override { return true; }

        editor_layer* m_editor_layer;
    };
} // namespace sgm

application* create_app_instance() { return new sgm::sgm_app; }