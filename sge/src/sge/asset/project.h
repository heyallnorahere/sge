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
#include "sge/asset/asset_manager.h"
#include "sge/scene/scene.h"
namespace sge {
    class project {
    public:
        static void init(bool editor);
        static void shutdown();

        static std::string get_config();
        static std::string get_cpu_architecture();

        static bool loaded();
        static project& get();

        static bool save();
        static bool load(const fs::path& path);
        static void reload_assembly(const std::vector<ref<scene>>& active_scenes);

        project(const project&) = delete;
        project& operator=(const project&) = delete;

        asset_manager& get_asset_manager() { return *m_asset_manager; }

        const std::string& get_name() { return m_name; }
        const fs::path& get_path() { return m_path; }

        fs::path get_directory() { return m_path.parent_path(); }
        fs::path get_asset_dir() { return get_directory() / m_asset_dir; }
        fs::path get_start_scene() { return get_asset_dir() / m_start_scene; }
        fs::path get_script_project_path() { return get_directory() / "ScriptAssembly.csproj"; }

        fs::path get_assembly_path() {
            return get_directory() / "bin" / get_config() / "net6.0" / "ScriptAssembly.dll";
        }

        std::optional<size_t> get_assembly_index() { return m_assembly_index; }

    private:
        project() = default;

        std::unique_ptr<asset_manager> m_asset_manager;
        fs::path m_path, m_asset_dir, m_start_scene;
        std::string m_name;
        std::optional<size_t> m_assembly_index;
    };
} // namespace sge
