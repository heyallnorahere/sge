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
#include "sge/scene/prefab.h"
#include "sge/scene/scene_serializer.h"
#include "sge/asset/project.h"

namespace sge {
    static entity_serializer s_prefab_serializer = entity_serializer(false);
    ref<prefab> prefab::from_entity(entity e, const fs::path& path) {
        json data;
        try {
            s_prefab_serializer.serialize(data, e);
        } catch (const std::exception&) {
            return nullptr;
        }

        auto& _project = project::get();
        fs::path asset_dir = _project.get_asset_dir();

        fs::path asset_path = path;
        if (!asset_path.empty() && asset_path.is_relative()) {
            asset_path = asset_dir / asset_path;
        }

        ref<prefab> result = new prefab(asset_path, data);
        if (!asset_path.empty()) {
            if (!serialize(result, asset_path)) {
                result.reset();
            } else {
                auto& registry = _project.get_asset_manager().registry;
                fs::path relative_path = fs::relative(asset_path, asset_dir);

                if (registry.contains(relative_path)) {
                    registry.remove_asset(relative_path);
                }

                registry.register_asset(result);
            }
        }

        return result;
    }

    bool prefab::serialize(ref<prefab> _prefab, const fs::path& path) {
        std::ofstream stream(path);
        if (!stream.is_open()) {
            return false;
        }

        stream << _prefab->m_data.dump(4) << std::flush;
        stream.close();

        return true;
    }

    prefab::prefab(const fs::path& path) : m_path(path) {
        if (!reload()) {
            throw std::runtime_error("initial load failed!");
        }
    }

    bool prefab::reload() {
        if (!fs::exists(m_path)) {
            spdlog::error("attempted to load prefab from nonexistent file: {0}", m_path.string());
            return false;
        }

        json data;
        try {
            std::ifstream stream(m_path);
            stream >> data;
            stream.close();
        } catch (const std::exception&) {
            return false;
        }

        m_data = data;
        return true;
    }

    entity prefab::instantiate(ref<scene> _scene) {
        return s_prefab_serializer.deserialize(m_data, _scene);
    }
} // namespace sge