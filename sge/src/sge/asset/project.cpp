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
namespace sge {
    static std::unique_ptr<project> global_project;

    void project::init() {
        auto instance = new project;
        instance->m_asset_manager = std::make_unique<asset_manager>();

        global_project = std::unique_ptr<project>(instance);
    }

    void project::shutdown() { global_project.reset(); }

    bool project::loaded() { return !global_project->m_path.empty(); }
    project& project::get() { return *global_project; }

    bool project::save() {
        if (!loaded()) {
            return false;
        }

        fs::path directory = global_project->get_directory();
        fs::path registry_path = global_project->m_asset_manager->registry.get_path();
        fs::path relative_registry_path = registry_path.lexically_relative(directory);

        json data;
        data["name"] = global_project->m_name;
        data["asset_registry"] = relative_registry_path;
        data["asset_directory"] = global_project->m_asset_dir;
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


        auto registry_path = data["asset_registry"].get<fs::path>();
        if (registry_path.is_absolute()) {
            registry_path = registry_path.lexically_relative(directory);
        }
        global_project->m_asset_manager->set_path(registry_path);

        auto asset_dir = data["asset_directory"].get<fs::path>();
        if (asset_dir.is_absolute()) {
            asset_dir = asset_dir.lexically_relative(directory);
        }
        global_project->m_asset_dir = asset_dir;

        auto start_scene = data["start_scene"].get<fs::path>();
        if (start_scene.is_absolute()) {
            start_scene = start_scene.lexically_relative(asset_dir);
        }
        global_project->m_start_scene = start_scene;

        return true;
    }
} // namespace sge