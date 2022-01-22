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
#include "sge/asset/asset_registry.h"
#include "sge/asset/json.h"
namespace sge {
    void to_json(json& data, const asset_registry::asset_desc& desc) {
        data["guid"] = nullptr;
        data["path"] = desc.path;
        data["type"] = nullptr;

        if (desc.type.has_value()) {
            std::string type_string;
            switch (desc.type.value()) {
            case asset_type::shader:
                type_string = "shader";
                break;
            case asset_type::texture_2d:
                type_string = "texture_2d";
                break;
            default:
                throw std::runtime_error("invalid asset type!");
            }

            data["type"] = type_string;
        }

        if (desc.id.has_value()) {
            data["guid"] = desc.id.value();
        }
    }

    static const std::unordered_map<std::string, asset_type> asset_type_map = {
        { "shader", asset_type::shader }, { "texture_2d", asset_type::texture_2d }
    };

    void from_json(const json& data, asset_registry::asset_desc& desc) {
        auto id_node = data["guid"];
        if (!id_node.is_null()) {
            desc.id = id_node.get<guid>();
        }

        desc.path = data["path"].get<fs::path>();

        auto type_node = data["type"];
        if (!type_node.is_null()) {
            auto type_string = type_node.get<std::string>();
            if (asset_type_map.find(type_string) == asset_type_map.end()) {
                throw std::runtime_error("invalid asset type: " + type_string);
            }

            asset_type type = asset_type_map.at(type_string);
            desc.type = type;
        }
    }

    asset_registry::asset_registry(const fs::path& path) {
        m_path = path;

        load();
    }

    void asset_registry::load() {
        m_path_map.clear();
        m_id_map.clear();

        if (!fs::exists(m_path)) {
            spdlog::warn("attempted to load a nonexistent registry!");
            return;
        }

        json data;

        std::ifstream stream(m_path);
        stream >> data;
        stream.close();

        for (json node : data) {
            auto desc = node.get<asset_desc>();
            size_t index = find_next_available_index();

            if (m_path_map.find(desc.path) != m_path_map.end()) {
                spdlog::warn("path {0} is registered twice!", desc.path.string());
                continue;
            }

            if (!fs::exists(desc.path)) {
                spdlog::warn("path {0} does not exist!", desc.path.string());
                continue;
            }

            if (desc.id.has_value()) {
                guid id = desc.id.value();

                if (m_id_map.find(id) != m_id_map.end()) {
                    spdlog::warn("id {0} is registered twice!", (uint64_t)id);
                    continue;
                } else {
                    m_id_map.insert(std::make_pair(id, index));
                }
            }

            m_assets.insert(std::make_pair(index, desc));
            m_path_map.insert(std::make_pair(desc.path, index));
        }
    }

    void asset_registry::save() {
        json data = "[]"_json;
        for (const auto& desc : m_assets) {
            data.push_back(desc);
        }

        std::ofstream stream(m_path);
        stream << data.dump(4) << std::flush;
        stream.close();
    }

    bool asset_registry::contains(const fs::path& path) {
        return m_path_map.find(path) != m_path_map.end();
    }

    bool asset_registry::contains(guid id) { return m_id_map.find(id) != m_id_map.end(); }

    bool asset_registry::register_asset(ref<asset> _asset) {
        fs::path path = _asset->get_path();
        if (path.empty() || (m_path_map.find(path) != m_path_map.end())) {
            return false;
        }

        if (m_id_map.find(_asset->id) != m_id_map.end()) {
            return false;
        }

        asset_desc desc;
        desc.path = path;
        desc.id = _asset->id;
        desc.type = _asset->get_asset_type();

        size_t index = find_next_available_index();
        m_assets.insert(std::make_pair(index, desc));

        m_path_map.insert(std::make_pair(path, index));
        m_id_map.insert(std::make_pair(_asset->id, index));

        return true;
    }

    bool asset_registry::register_asset(const fs::path& path, std::optional<guid> id) {
        if (m_path_map.find(path) != m_path_map.end()) {
            return false;
        }

        asset_desc desc;
        desc.path = path;

        if (id.has_value()) {
            guid id_value = id.value();

            if (m_id_map.find(id_value) != m_id_map.end()) {
                return false;
            } else {
                desc.id = id_value;
            }
        }

        size_t index = find_next_available_index();
        m_assets.insert(std::make_pair(index, desc));
        m_path_map.insert(std::make_pair(path, index));

        if (id.has_value()) {
            m_id_map.insert(std::make_pair(id.value(), index));
        }

        return true;
    }

    bool asset_registry::remove_asset(const fs::path& path) {
        if (m_path_map.find(path) == m_path_map.end()) {
            return false;
        }

        size_t index = m_path_map[path];
        m_path_map.erase(path);

        auto desc = m_assets[index];
        m_assets.erase(index);

        if (desc.id.has_value()) {
            m_id_map.erase(desc.id.value());
        }

        return true;
    }

    bool asset_registry::remove_asset(guid id) {
        if (m_id_map.find(id) == m_id_map.end()) {
            return false;
        }

        size_t index = m_id_map[id];
        m_id_map.erase(id);

        auto desc = m_assets[index];
        m_assets.erase(index);

        m_path_map.erase(desc.path);
        return true;
    }

    size_t asset_registry::get(const fs::path& path) {
        if (m_path_map.find(path) == m_path_map.end()) {
            return (size_t)-1;
        }

        return m_path_map[path];
    }

    size_t asset_registry::get(guid id) {
        if (m_id_map.find(id) == m_id_map.end()) {
            return (size_t)-1;
        }

        return m_id_map[id];
    }

    bool asset_registry::get(size_t index, asset_desc& desc) {
        if (m_assets.find(index) == m_assets.end()) {
            return false;
        }

        desc = m_assets[index];
        return true;
    }
} // namespace sge