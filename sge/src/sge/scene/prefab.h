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
#include "sge/asset/json.h"
#include "sge/scene/scene.h"
#include "sge/scene/entity.h"

namespace sge {
    class prefab : public asset {
    public:
        static ref<prefab> from_entity(entity e, const fs::path& path);
        static bool serialize(ref<prefab> _prefab, const fs::path& path);

        prefab(const fs::path& path) : m_path(path) { reload(); }
        virtual ~prefab() override = default;

        virtual asset_type get_asset_type() override { return asset_type::prefab; }
        virtual const fs::path& get_path() override { return m_path; }

        virtual bool reload() override;

        entity instantiate(ref<scene> _scene);

    private:
        prefab(const fs::path& path, const json& data) : m_path(path), m_data(data) {}

        fs::path m_path;
        json m_data;
    };
} // namespace sge