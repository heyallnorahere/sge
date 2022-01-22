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
    class asset_registry {
    public:
        struct asset_desc {
            std::optional<guid> id;
            fs::path path;
            std::optional<asset_type> type;
        };

        asset_registry(const fs::path& path);
        ~asset_registry() { save(); }

        asset_registry(const asset_registry&) = delete;
        asset_registry& operator=(const asset_registry&) = delete;

        void load();
        void save();

        bool contains(const fs::path& path);
        bool contains(guid id);

        bool register_asset(ref<asset> _asset);
        bool register_asset(const fs::path& path, std::optional<guid> id = std::optional<guid>());

        bool remove_asset(const fs::path& path);
        bool remove_asset(guid id);

        size_t get(const fs::path& path);
        size_t get(guid id);
        bool get(size_t index, asset_desc& desc);

    private:
        size_t find_next_available_index();

        std::unordered_map<fs::path, size_t> m_path_map;
        std::unordered_map<guid, size_t> m_id_map;
        std::unordered_map<size_t, asset_desc> m_assets;

        fs::path m_path;
    };
} // namespace sge