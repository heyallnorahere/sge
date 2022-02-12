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
            [this](asset_registry::registry_action action, const fs::path& path) {
                switch (action) {
                case asset_registry::registry_action::add:
                    // nothing
                    break;
                case asset_registry::registry_action::remove:
                    if (m_cache.find(path) != m_cache.end()) {
                        m_cache.erase(path);
                    }
                    break;
                case asset_registry::registry_action::clear:
                    m_cache.clear();
                    break;
                default:
                    throw std::runtime_error("invalid registry action!");
                }
            });
    }

    ref<asset> asset_manager::get_asset(const fs::path& path) {
        if (m_cache.find(path) == m_cache.end()) {
            if (registry.contains(path)) {
                const auto& desc = registry[path];
                if (desc.type.has_value()) {
                    ref<asset> _asset;
                    if (asset_serializer::deserialize(desc, _asset)) {
                        m_cache.insert(std::make_pair(path, _asset));
                        return _asset;
                    } else {
                        m_cache.insert(std::make_pair(path, nullptr));
                    }
                }
            }

            return nullptr;
        }

        return m_cache[path];
    }
} // namespace sge
