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
#include "sge/asset/asset_registry.h"
namespace sge {
    class asset_manager {
    public:
        asset_registry registry;

        asset_manager(const fs::path& registry_path);
        ~asset_manager() = default;

        asset_manager(const asset_manager&) = delete;
        asset_manager operator=(const asset_manager&) = delete;

        ref<asset> get_asset(const fs::path& path);

    private:
        std::unordered_map<fs::path, ref<asset>> m_cache;

        // todo: serializers
    };
} // namespace sge
