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
#include "sge/asset/asset_manager.h"
#include "sge/asset/asset_serializers.h"
namespace sge {
    asset_manager::asset_manager() {
        registry.set_on_changed_callback(
            [this](asset_registry::registry_action action, const fs::path& path) mutable {
                switch (action) {
                case asset_registry::registry_action::add:
                    // nothing
                    break;
                case asset_registry::registry_action::remove:
                    if (m_path_cache.find(path) != m_path_cache.end()) {
                        auto _asset = m_path_cache[path];
                        if (_asset && m_guid_cache.find(_asset->id) != m_guid_cache.end()) {
                            m_guid_cache.erase(_asset->id);
                        }

                        m_path_cache.erase(path);
                    }

                    break;
                case asset_registry::registry_action::clear:
                    m_path_cache.clear();
                    m_guid_cache.clear();

                    break;
                default:
                    throw std::runtime_error("invalid registry action!");
                }
            });
    }

    ref<asset> asset_manager::get_asset(const fs::path& path) {
        if (m_path_cache.find(path) == m_path_cache.end()) {
            if (registry.contains(path)) {
                const auto& desc = registry[path];

                if (desc.type.has_value() && desc.id.has_value()) {
                    ref<asset> _asset;

                    if (asset_serializer::deserialize(desc, _asset)) {
                        m_path_cache.insert(std::make_pair(path, _asset));
                        m_guid_cache.insert(std::make_pair(_asset->id, _asset));

                        return _asset;
                    } else {
                        m_path_cache.insert(std::make_pair(path, nullptr));
                        m_guid_cache.insert(std::make_pair(desc.id.value(), nullptr));
                    }
                }
            }

            return nullptr;
        }

        return m_path_cache[path];
    }

    ref<asset> asset_manager::get_asset(guid id) {
        if (m_guid_cache.find(id) != m_guid_cache.end()) {
            return m_guid_cache[id];
        }

        for (const auto& [path, desc] : registry) {
            if (desc.id == id && desc.type.has_value()) {
                ref<asset> _asset;

                if (asset_serializer::deserialize(desc, _asset)) {
                    m_path_cache.insert(std::make_pair(path, _asset));
                    m_guid_cache.insert(std::make_pair(id, _asset));

                    return _asset;
                } else {
                    m_path_cache.insert(std::make_pair(path, nullptr));
                    m_guid_cache.insert(std::make_pair(id, nullptr));

                    break;
                }
            }
        }

        return nullptr;
    }
} // namespace sge
