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
#include "sge/script/script_engine.h"
#ifdef SGE_PLATFORM_WINDOWS
#define NOMINMAX
#include <Windows.h>
#endif
namespace sge {
    static std::unique_ptr<project> global_project;

    void project::init() {
        auto instance = new project;
        instance->m_asset_manager = std::make_unique<asset_manager>();

        global_project = std::unique_ptr<project>(instance);
    }

    void project::shutdown() { global_project.reset(); }

    std::string project::get_config() {
#ifdef SGE_DEBUG
        return "Debug";
#else
        return "Release";
#endif
    }

    bool project::loaded() { return !global_project->m_path.empty(); }
    project& project::get() { return *global_project; }

    bool project::save() {
        if (!loaded()) {
            return false;
        }

        fs::path directory = global_project->get_directory();
        fs::path registry_path = global_project->m_asset_manager->registry.get_path();
        if (registry_path.is_absolute()) {
            registry_path = registry_path.lexically_relative(directory);
        }

        json data;
        data["name"] = global_project->m_name;
        data["asset_directory"] = global_project->m_asset_dir;
        data["asset_registry"] = registry_path;
        data["start_scene"] = global_project->m_start_scene;

        std::ofstream stream(global_project->m_path);
        stream << data.dump(4) << std::flush;
        stream.close();

        global_project->m_asset_manager->registry.save();
        return true;
    }

    bool project::load(const fs::path& path) {
        fs::path project_path = path;
        if (project_path.is_relative()) {
            project_path = fs::current_path() / project_path;
        }

        if (!fs::exists(project_path)) {
            return false;
        }

        json data;
        try {
            std::ifstream stream(project_path);
            stream >> data;
            stream.close();
        } catch (const std::exception& exc) {
            spdlog::warn("could not load project {0}: {1}", project_path.string(), exc.what());
            return false;
        }

        global_project->m_name = data["name"].get<std::string>();
        global_project->m_path = project_path;
        fs::path directory = global_project->get_directory();

        auto asset_dir = data["asset_directory"].get<fs::path>();
        if (asset_dir.is_absolute()) {
            asset_dir = asset_dir.lexically_relative(directory);
        }
        global_project->m_asset_dir = asset_dir;

        auto registry_path = data["asset_registry"].get<fs::path>();
        if (registry_path.is_relative()) {
            registry_path = directory / registry_path;
        }
        global_project->m_asset_manager->set_path(registry_path);

        auto start_scene = data["start_scene"].get<fs::path>();
        if (start_scene.is_absolute()) {
            start_scene = start_scene.lexically_relative(asset_dir);
        }
        global_project->m_start_scene = start_scene;

        reload_assembly();
        return true;
    }

#ifdef SGE_PLATFORM_WINDOWS
    static int32_t run_command(const fs::path& executable, const std::string& cmdline) {
        char* buffer = (char*)malloc((cmdline.length() + 1) * sizeof(char));
        strcpy(buffer, cmdline.c_str());

        STARTUPINFOA startup_info;
        memset(&startup_info, 0, sizeof(STARTUPINFOA));
        startup_info.cb = sizeof(STARTUPINFOA);

        int32_t exit_code = -1;
        std::string exe_string = executable.string();
        PROCESS_INFORMATION info;
        if (::CreateProcessA(exe_string.c_str(), buffer, nullptr, nullptr, true, 0, nullptr,
                             nullptr, &startup_info, &info)) {
            ::WaitForSingleObject(info.hProcess, std::numeric_limits<DWORD>::max());

            DWORD win32_exit_code = 0;
            if (!::GetExitCodeProcess(info.hProcess, &win32_exit_code)) {
                throw std::runtime_error("could not get exit code from executed command!");
            }

            ::CloseHandle(info.hProcess);
            ::CloseHandle(info.hThread);

            exit_code = (int32_t)win32_exit_code;
        }

        free(buffer);
        return exit_code;
    }
#else
    static int32_t run_command(const fs::path& executable, const std::string& cmdline) {
        FILE* pipe = popen(cmdline.c_str(), "r");
        if (pipe == nullptr) {
            return -1;
        }

        return pclose(pipe);
    }
#endif

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

        std::string command = command_stream.str();
        int32_t return_code = run_command(executable_path, command);
        if (return_code != 0) {
            spdlog::warn("MSBuild exited with code: {0}", return_code);
            return false;
        }

        return true;
    }

    std::optional<size_t> project::reload_assembly() {
        std::optional<size_t> new_index;

        if (compile_app_assembly()) {
            fs::path current_path = global_project->get_assembly_path();
            if (global_project->m_assembly_index.has_value()) {
                size_t old_index = global_project->m_assembly_index.value();

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

        if (global_project->m_assembly_index.has_value() && !new_index.has_value()) {
            size_t index = global_project->m_assembly_index.value();
            script_engine::unload_assembly(index);
        }

        global_project->m_assembly_index = new_index;
        return new_index;
    }
} // namespace sge
