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
#include "sge/asset/project.h"
namespace sge {
    static const std::unordered_map<std::string, asset_type> asset_type_map = {
        { "shader", asset_type::shader },
        { "texture_2d", asset_type::texture_2d },
        { "prefab", asset_type::prefab },
        { "sound", asset_type::sound }
    };

    static const std::string& get_asset_type_name(asset_type type) {
        static std::unordered_map<asset_type, std::string> type_name_map;
        if (type_name_map.empty()) {
            for (const auto& [name, type] : asset_type_map) {
                type_name_map.insert(std::make_pair(type, name));
            }
        }

        if (type_name_map.find(type) == type_name_map.end()) {
            throw std::runtime_error("invalid asset type!");
        }

        return type_name_map.at(type);
    }

    void to_json(json& data, const asset_desc& desc) {
        data["guid"] = nullptr;
        data["path"] = desc.path;
        data["type"] = nullptr;

        if (desc.type.has_value()) {
            data["type"] = get_asset_type_name(desc.type.value());
        }

        if (desc.id.has_value()) {
            data["guid"] = desc.id.value();
        }
    }

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

    void asset_registry::load() {
        m_mutex.lock();
        m_assets.clear();

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

            fs::path asset_dir = project::get().get_asset_dir();
            fs::path abs_path = desc.path;
            if (abs_path.is_relative()) {
                abs_path = asset_dir / abs_path;
            }

            if (!fs::exists(abs_path)) {
                spdlog::warn("path {0} does not exist!", abs_path.string());
                continue;
            }

            m_assets.insert(std::make_pair(desc.path, desc));
        }

        on_changed_callback callback;
        if (m_on_changed_callback) {
            callback = m_on_changed_callback;
        }

        m_mutex.unlock();
        if (callback) {
            callback(registry_action::clear, m_path);
        }
    }

    void asset_registry::save() {
        m_mutex.lock();

        json data;
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
        if (path.is_absolute()) {
            fs::path asset_dir = project::get().get_asset_dir();
            path = path.lexically_relative(asset_dir);
        }

        if (path.empty() || (m_assets.find(path) != m_assets.end())) {
            m_mutex.unlock();
            return false;
        }

        asset_desc desc;
        desc.path = path;
        desc.id = _asset->id;
        desc.type = _asset->get_asset_type();
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

    bool asset_registry::register_asset(const fs::path& path) {
        m_mutex.lock();

        fs::path asset_path = path;
        if (asset_path.is_absolute()) {
            fs::path asset_dir = project::get().get_asset_dir();
            asset_path = asset_path.lexically_relative(asset_dir);
        }

        if (asset_path.empty() || (m_assets.find(asset_path) != m_assets.end())) {
            m_mutex.unlock();
            return false;
        }

        asset_desc desc;
        desc.path = asset_path;
        m_assets.insert(std::make_pair(asset_path, desc));

        on_changed_callback callback;
        if (m_on_changed_callback) {
            callback = m_on_changed_callback;
        }

        m_mutex.unlock();
        save();

        if (callback) {
            callback(registry_action::add, asset_path);
        }

        return true;
    }

    bool asset_registry::remove_asset(const fs::path& path) {
        m_mutex.lock();

        fs::path asset_path = path;
        if (asset_path.is_absolute()) {
            fs::path asset_dir = project::get().get_asset_dir();
            asset_path = asset_path.lexically_relative(asset_dir);
        }

        if (m_assets.find(asset_path) == m_assets.end()) {
            m_mutex.unlock();
            return false;
        }

        on_changed_callback callback;
        if (m_on_changed_callback) {
            callback = m_on_changed_callback;
        }

        m_assets.erase(asset_path);
        m_mutex.unlock();

        save();

        if (callback) {
            callback(registry_action::remove, asset_path);
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
