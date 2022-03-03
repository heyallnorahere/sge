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

#include "sgepch.h"
#include "sge/asset/project.h"
#include "sge/asset/json.h"
#include "sge/core/environment.h"
#include "sge/script/script_engine.h"
#include "sge/script/state_preserver.h"
namespace sge {
    struct project_data_t {
        std::unique_ptr<project> instance;
        bool editor;
    };
    static std::unique_ptr<project_data_t> project_data;

    void project::init(bool editor) {
        if (project_data) {
            spdlog::warn("projects have already been initialized");
            return;
        }

        auto instance = new project;
        instance->m_asset_manager = std::make_unique<asset_manager>();

        project_data = std::make_unique<project_data_t>();
        project_data->instance = std::unique_ptr<project>(instance);
        project_data->editor = editor;
    }

    void project::shutdown() {
        if (!project_data) {
            spdlog::warn("projects have not been initalized!");
        }

        project_data.reset();
    }

    std::string project::get_config() {
#ifdef SGE_DEBUG
        return "Debug";
#else
        return "Release";
#endif
    }

    std::string project::get_cpu_architecture() { return SGE_CPU_ARCHITECTURE; }

    bool project::loaded() { return !project_data->instance->m_path.empty(); }
    project& project::get() { return *project_data->instance; }

    bool project::save() {
        if (!loaded()) {
            return false;
        }

        auto& instance = get();
        fs::path directory = instance.get_directory();
        fs::path registry_path = instance.m_asset_manager->registry.get_path();
        if (registry_path.is_absolute()) {
            registry_path = registry_path.lexically_relative(directory);
        }

        json data;
        data["name"] = instance.m_name;
        data["asset_directory"] = instance.m_asset_dir;
        data["asset_registry"] = registry_path;
        data["start_scene"] = instance.m_start_scene;

        std::ofstream stream(instance.m_path);
        stream << data.dump(4) << std::flush;
        stream.close();

        instance.m_asset_manager->registry.save();
        return true;
    }

    bool project::load(const fs::path& path) {
        fs::path project_path = path;
        if (project_path.is_relative()) {
            project_path = fs::current_path() / project_path;
        }

        if (!fs::exists(project_path)) {
            spdlog::warn("project does not exist: {0}", path.string());
            return false;
        }

        json data;
        try {
            std::ifstream stream(project_path);
            stream >> data;
            stream.close();
        } catch (const std::exception& exc) {
            spdlog::warn("invalid project {0}: {1}", project_path.string(), exc.what());
            return false;
        }

        auto& instance = get();
        instance.m_name = data["name"].get<std::string>();
        instance.m_path = project_path;
        fs::path directory = instance.get_directory();

        auto asset_dir = data["asset_directory"].get<fs::path>();
        if (asset_dir.is_absolute()) {
            asset_dir = asset_dir.lexically_relative(directory);
        }
        instance.m_asset_dir = asset_dir;

        auto registry_path = data["asset_registry"].get<fs::path>();
        if (registry_path.is_relative()) {
            registry_path = directory / registry_path;
        }
        instance.m_asset_manager->set_path(registry_path);

        auto start_scene = data["start_scene"].get<fs::path>();
        if (start_scene.is_absolute()) {
            start_scene = start_scene.lexically_relative(asset_dir);
        }
        instance.m_start_scene = start_scene;

        reload_assembly();
        return true;
    }

    static bool compile_app_assembly() {
        static const std::vector<fs::path> dotnet_executable_names = { "dotnet", "dotnet.exe" };
        static const std::vector<fs::path> dotnet_search_paths = {
            "C:\\Program Files",
            "C:\\Program Files (x86)",
            "/usr/share",
            "/usr/local/share",
        };

        static std::optional<fs::path> dotnet_executable_path;
        if (!dotnet_executable_path.has_value()) {
            for (const fs::path& current_path : dotnet_search_paths) {
                for (const fs::path& current_name : dotnet_executable_names) {
                    fs::path executable_path = current_path / "dotnet" / current_name;

                    if (fs::exists(executable_path)) {
                        dotnet_executable_path = executable_path;
                        break;
                    }
                }

                if (dotnet_executable_path.has_value()) {
                    break;
                }
            }

            if (!dotnet_executable_path.has_value()) {
                spdlog::warn("could not find .NET Core executable!");
                return false;
            }
        }

        fs::path executable_path = dotnet_executable_path.value();
        std::stringstream command_stream;
        command_stream << "\"" << executable_path.string() << "\" build \""
                       << project::get().get_script_project_path().string() << "\" -c "
                       << project::get_config();

        process_info command_info;
        command_info.executable = executable_path;
        command_info.cmdline = command_stream.str();
        command_info.output_file = "assets/logs/dotnet.log";

        environment::run_command(command_info);
        return true;
    }

    std::optional<size_t> project::reload_assembly() {
        std::optional<size_t> new_index;

        bool load = true;
        if (project_data->editor) {
            load = compile_app_assembly();
        }

        auto& instance = get();
        /*std::unique_ptr<state_preserver> preserver;
        if (instance.m_assembly_index.has_value()) {
            void* assembly = script_engine::get_assembly(instance.m_assembly_index.value());
            std::vector<void*> assemblies = { assembly };
            
            preserver = std::make_unique<state_preserver>(assemblies);
        }*/

        if (load) {
            fs::path current_path = instance.get_assembly_path();
            if (instance.m_assembly_index.has_value()) {
                size_t old_index = instance.m_assembly_index.value();

                if (script_engine::get_assembly_path(old_index) != current_path) {
                    script_engine::unload_assembly(old_index);
                    new_index = script_engine::load_assembly(current_path);
                } else {
                    new_index = script_engine::reload_assembly(old_index);
                }
            } else {
                new_index = script_engine::load_assembly(current_path);
            }
        }

        if (instance.m_assembly_index.has_value() && !new_index.has_value()) {
            size_t index = instance.m_assembly_index.value();
            script_engine::unload_assembly(index);
        }

        instance.m_assembly_index = new_index;
        return new_index;
    }
} // namespace sge
