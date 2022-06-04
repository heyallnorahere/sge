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

#include "launcher_pch.h"
#include "launcher_layer.h"
#include <sge/core/main.h>

namespace sgm::launcher {
    class sgm_launcher : public application {
    public:
        sgm_launcher() : application("SGM Launcher") {
            static const std::vector<subsystem> disabled_subsystems = { subsystem_script_engine,
                                                                        subsystem_project };

            for (subsystem id : disabled_subsystems) {
                disable_subsystem(id);
            }
        }

    protected:
        virtual void on_init() {
            launcher_layer::app_callbacks callbacks;
            callbacks.create_project = SGE_BIND_EVENT_FUNC(sgm_launcher::create_project);
            callbacks.open_project = SGE_BIND_EVENT_FUNC(sgm_launcher::open_project);

            m_layer = new launcher_layer(callbacks);
            push_layer(m_layer);
        }

        virtual void on_shutdown() {
            pop_layer(m_layer);
            delete m_layer;
        }

        virtual fs::path get_imgui_config_path() override {
            return fs::current_path() / "launcher.ini";
        }

    private:
        void migrate_file(const fs::path& src, const fs::path& dst,
                          const std::string& project_name) {
            fs::path dst_directory = dst.parent_path();
            if (!fs::exists(dst_directory)) {
                fs::create_directories(dst_directory);
            }

            fs::path extension = src.extension();
            if (extension == ".png" || extension == ".jpg" || extension == ".jpeg") {
                fs::copy_file(src, dst);
                return;
            }

            std::stringstream contents_stream;
            std::ifstream in_stream(src);
            std::string line;

            while (std::getline(in_stream, line)) {
                contents_stream << line << '\n';
            }

            in_stream.close();
            std::string contents = contents_stream.str();

            static const std::string project_token = "%project%";
            size_t pos = 0;
            while ((pos = contents.find(project_token, pos)) != std::string::npos) {
                contents.replace(pos, project_token.length(), project_name);
                pos += project_name.length();
            }

            fs::path directory = dst.parent_path();
            if (!fs::exists(directory)) {
                fs::create_directories(directory);
            }

            std::ofstream out_stream(dst);
            out_stream << contents << std::flush;
            out_stream.close();
        }

        bool create_project(const launcher_layer::project_info_t& project_info,
                            std::string& error) {
            std::string project_name = project_info.name;
            for (size_t i = 0; i < project_name.length(); i++) {
                // i dont know how else to do this
                char c = project_name[i];
                bool is_digit = (c >= '0' && c <= '9');
                bool is_lowercase = (c >= 'a' && c <= 'z');
                bool is_uppercase = (c >= 'A' && c <= 'Z');

                if (is_digit && i == 0) {
                    error = "The project name cannot start with a digit!";
                    return false;
                }

                if (!is_digit && !is_lowercase && !is_uppercase) {
                    error = "The project name must only comprise of letters and numbers!";
                    return false;
                }
            }

            fs::path project_path = project_info.path;
            fs::path directory = project_path.parent_path();

            if (fs::exists(directory)) {
                for (const auto& entry : fs::directory_iterator(directory)) {
                    error = "The directory in which the project will be created is already in use!";
                    return false;
                }
            }

            fs::path template_dir = fs::current_path() / "assets" / "template";
            fs::path src_project_file = template_dir / "Template.sgeproject";

            if (!fs::exists(template_dir)) {
                error = "The template project is not present!";
                return false;
            }

            spdlog::info("creating project: {0}", project_path.string());
            for (const auto& entry : fs::recursive_directory_iterator(template_dir)) {
                if (entry.is_directory()) {
                    continue;
                }

                fs::path src_path = entry.path();
                if (src_path == src_project_file) {
                    continue;
                }

                fs::path relative_path = src_path.lexically_relative(template_dir);
                fs::path dst_path = directory / relative_path;

                migrate_file(src_path, dst_path, project_name);
            }

            migrate_file(src_project_file, project_path, project_name);
            spdlog::info("successfully created project");

            error.clear();
            return true;
        }

        void open_project(const fs::path& project_path) {
            spdlog::info("opening project: {0}", project_path.string());

            static fs::path sgm_path;
            if (sgm_path.empty()) {
                sgm_path = fs::current_path() / "bin";
#ifdef SGE_DEBUG
                sgm_path /= "Debug";
#endif

                std::string sgm_name = "sgm";
#ifdef SGE_PLATFORM_WINDOWS
                sgm_name += ".exe";
#endif

                sgm_path /= sgm_name;
            }

            std::stringstream command;
            command << std::quoted(sgm_path.string()) << " " << std::quoted(project_path.string());

            process_info info;
            info.executable = sgm_path;
            info.cmdline = command.str();
            info.workdir = fs::current_path();
            info.output_file = "assets/logs/sgm.log";
            info.detach = true;

            int32_t return_code = environment::run_command(info);
            if (return_code != 0) {
                spdlog::error("could not launch SGM! (error code: {0})", return_code);
            } else {
                m_layer->add_to_recent(project_path);
            }
        }

        launcher_layer* m_layer;
    };
} // namespace sgm::launcher

application* create_app_instance() { return new sgm::launcher::sgm_launcher; }
