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
    void to_json(json& data, const asset_desc& desc) {
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

    void from_json(const json& data, asset_desc& desc) {
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

    asset_registry::~asset_registry() {
        if (!m_path.empty()) {
            save();
        }
    }

    void asset_registry::load() {
        clear();

        m_mutex.lock();
        if (!fs::exists(m_path)) {
            spdlog::warn("attempted to load a nonexistent registry!");

            m_mutex.unlock();
            return;
        }

        json data;

        std::ifstream stream(m_path);
        stream >> data;
        stream.close();

        for (json node : data) {
            auto desc = node.get<asset_desc>();
            if (m_assets.find(desc.path) != m_assets.end()) {
                spdlog::warn("path {0} is registered twice!", desc.path.string());
                continue;
            }

            if (!fs::exists(desc.path)) {
                spdlog::warn("path {0} does not exist!", desc.path.string());
                continue;
            }

            m_assets.insert(std::make_pair(desc.path, desc));
        }

        m_mutex.unlock();
    }

    void asset_registry::save() {
        m_mutex.lock();

        json data = "[]"_json;
        for (const auto& [path, desc] : m_assets) {
            data.push_back(desc);
        }

        std::ofstream stream(m_path);
        stream << data.dump(4) << std::flush;
        stream.close();

        m_mutex.unlock();
    }

    bool asset_registry::register_asset(ref<asset> _asset) {
        m_mutex.lock();
        fs::path path = _asset->get_path();
        if (path.empty() || (m_assets.find(path) != m_assets.end())) {
            m_mutex.unlock();
            return false;
        }

        asset_desc desc;
        desc.path = path;
        desc.id = _asset->id;
        desc.type = _asset->get_asset_type();
        m_assets.insert(std::make_pair(desc.path, desc));

        on_changed_callback callback;
        if (m_on_changed_callback) {
            callback = m_on_changed_callback;
        }

        m_mutex.unlock();
        save();

        if (callback) {
            callback(registry_action::add, path);
        }

        return true;
    }

    bool asset_registry::register_asset(const fs::path& path) {
        m_mutex.lock();
        if (path.empty() || (m_assets.find(path) != m_assets.end())) {
            m_mutex.unlock();
            return false;
        }

        asset_desc desc;
        desc.path = path;
        m_assets.insert(std::make_pair(path, desc));

        on_changed_callback callback;
        if (m_on_changed_callback) {
            callback = m_on_changed_callback;
        }

        m_mutex.unlock();
        save();

        if (callback) {
            callback(registry_action::add, path);
        }

        return true;
    }

    bool asset_registry::remove_asset(const fs::path& path) {
        m_mutex.lock();
        if (m_assets.find(path) == m_assets.end()) {
            m_mutex.unlock();
            return false;
        }

        on_changed_callback callback;
        if (m_on_changed_callback) {
            callback = m_on_changed_callback;
        }

        m_assets.erase(path);
        m_mutex.unlock();

        save();

        if (callback) {
            callback(registry_action::remove, path);
        }
        return true;
    }

    void asset_registry::clear() {
        m_mutex.lock();
        m_assets.clear();

        on_changed_callback callback;
        if (m_on_changed_callback) {
            callback = m_on_changed_callback;
        }

        m_mutex.unlock();
        save();

        if (callback) {
            callback(registry_action::clear, m_path);
        }
    }

    asset_desc asset_registry::operator[](const fs::path& path) {
        m_mutex.lock();
        if (m_assets.find(path) == m_assets.end()) {
            m_mutex.unlock();
            throw std::runtime_error("path " + path.string() + " is not registered!");
        }

        auto desc = m_assets[path];
        m_mutex.unlock();

        return desc;
    }

    void asset_registry::set_path(const fs::path& path) {
        m_mutex.lock();
        m_path = path;
        m_mutex.unlock();

        load();
    }

    void asset_registry::set_on_changed_callback(on_changed_callback callback) {
        m_mutex.lock();
        m_on_changed_callback = callback;
        m_mutex.unlock();
    }
} // namespace sge
