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
    static const std::string s_sgm_title =
        "Simple Game Maker v" + application::get_engine_version();

    class sgm_app : public application {
    public:
        sgm_app() : application("SGM") {}

        virtual bool is_editor() override { return true; }

    protected:
        virtual void pre_init() override {
            std::vector<std::string> args;
            get_application_args(args);

            // to set up sgm for debugging, set the command line arguments to the path of the
            // project you want to debug with
            if (args.size() < 2) {
                throw std::runtime_error("cannot run SGM without a project!");
            }

            // if --launched is passed, SGM will start the proxy debugger, SGE.Debugger.exe. to
            // debug the proxy debugger, as ironic as that sounds, do not pass this flag and debug
            // the proxy debugger manually.
            if (args.size() >= 3 && args[2] == "--launched") {
                std::string debugger_configuration;
#ifdef SGE_DEBUG
                debugger_configuration = "Debug";
#else
                debugger_configuration = "Release";
#endif

                fs::path debugger_path = fs::current_path() / "assets/assemblies" /
                                         debugger_configuration / "SGE.Debugger.exe";

#ifdef SGE_BUILD_DEBUGGER
                process_info p_info;
                p_info.detach = true;

#ifdef SGE_PLATFORM_WINDOWS
                p_info.executable = debugger_path;
                p_info.cmdline = debugger_path.filename().string();
#else
                p_info.executable = SGE_MSBUILD_EXE;
                p_info.cmdline = p_info.executable.string() + " \"" + debugger_path.string() + "\"";
#endif

                p_info.cmdline += " --address=" + std::string(SGE_DEBUGGER_AGENT_ADDRESS);
                p_info.cmdline += " --port=" + std::string(SGE_DEBUGGER_AGENT_PORT);

                environment::run_command(p_info);
#else
                spdlog::error("could not launch debugger at {0} (mono executable not found)",
                              debugger_path.string());
#endif
            }

            m_project_path = args[1];
        }

        virtual void on_init() override {
            spdlog::info("loading project: {0}", m_project_path.string());
            if (!project::load(fs::absolute(m_project_path))) {
                throw std::runtime_error("could not load project!");
            }

            project& _project = project::get();
            m_window->set_title(s_sgm_title + " - " + _project.get_name());

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

        virtual std::string get_window_title() override { return s_sgm_title; }
        virtual fs::path get_imgui_config_path() override { return fs::current_path() / "sgm.ini"; }

        virtual fs::path get_log_file_path() override {
            return fs::current_path() / "assets" / "logs" / "sgm.log";
        }

        editor_layer* m_editor_layer;
        fs::path m_project_path;
    };
} // namespace sgm

application* create_app_instance() { return new sgm::sgm_app; }
