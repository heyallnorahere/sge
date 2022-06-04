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
#include "sge/asset/asset.h"
namespace sge {
    class asset_manager;
    class asset_registry {
    public:
        asset_registry() = default;
        asset_registry(const fs::path& path) { set_path(path); }
        ~asset_registry() = default;

        asset_registry(const asset_registry&) = delete;
        asset_registry& operator=(const asset_registry&) = delete;

        void load();
        void save();

        bool register_asset(ref<asset> _asset);
        bool register_asset(const fs::path& path);
        bool remove_asset(const fs::path& path);
        void clear();

        bool contains(const fs::path& path) { return m_assets.find(path) != m_assets.end(); }
        asset_desc operator[](const fs::path& path);

        std::unordered_map<fs::path, asset_desc, path_hasher>::iterator begin() {
            return m_assets.begin();
        }

        std::unordered_map<fs::path, asset_desc, path_hasher>::iterator end() {
            return m_assets.end();
        }

        const fs::path& get_path() { return m_path; }

    private:
        enum class registry_action { add, remove, clear };

        using on_changed_callback = std::function<void(registry_action, fs::path)>;

        void set_path(const fs::path& path);
        void set_on_changed_callback(on_changed_callback callback);

        std::mutex m_mutex;
        std::unordered_map<fs::path, asset_desc, path_hasher> m_assets;

        on_changed_callback m_on_changed_callback;
        fs::path m_path;

        friend class asset_manager;
    };
} // namespace sge
