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
#include "sge/scene/shape.h"
#include "sge/asset/json.h"
#include "sge/asset/project.h"

namespace sge {
    void from_json(const json& data, shape_vertex& vertex) {
        data["position"].get_to(vertex.position);
        data["uv"].get_to(vertex.uv);
    }

    void to_json(json& data, const shape_vertex& vertex) {
        data["position"] = vertex.position;
        data["uv"] = vertex.uv;
    }

    static const std::unordered_map<std::string, shape_vertex_direction> s_vertex_direction_map = {
        { "clockwise", shape_vertex_direction::clockwise },
        { "counter_clockwise", shape_vertex_direction::counter_clockwise }
    };

    ref<shape> shape::create(const shape_desc& desc, const fs::path& path) {
        auto& _project = project::get();
        fs::path asset_dir = _project.get_asset_dir();

        fs::path asset_path = path;
        if (!asset_path.empty() && asset_path.is_relative()) {
            asset_path = asset_dir / asset_path;
        }

        ref<shape> result = new shape(asset_path, false);
        result->m_vertices = desc.vertices;
        result->m_direction = desc.direction;

        for (const auto& index_set : desc.indices) {
            if (!index_set.empty()) {
                result->m_indices.push_back(index_set);
            }
        }

        if (result->m_vertices.empty() || result->m_indices.empty()) {
            result.reset();
        } else if (!asset_path.empty()) {
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

    bool shape::serialize(ref<shape> _shape, const fs::path& path) {
        if (!_shape || path.empty()) {
            return false;
        }

        static std::unordered_map<shape_vertex_direction, std::string> direction_name_map;
        if (direction_name_map.empty()) {
            for (const auto& [name, direction] : s_vertex_direction_map) {
                direction_name_map.insert(std::make_pair(direction, name));
            }
        }

        if (direction_name_map.find(_shape->m_direction) == direction_name_map.end()) {
            return false;
        }

        json data;
        data["vertices"] = _shape->m_vertices;
        data["indices"] = _shape->m_indices;
        data["direction"] = _shape->m_direction;

        std::ofstream stream(path);
        if (!stream.is_open()) {
            return false;
        }

        stream << data.dump(4) << std::flush;
        stream.close();

        return true;
    }

    bool shape::reload() {
        if (!fs::exists(m_path)) {
            spdlog::error("attempted to load shape from nonexistent file: {0}", m_path.string());
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

        auto direction_name = data["direction"].get<std::string>();
        if (s_vertex_direction_map.find(direction_name) != s_vertex_direction_map.end()) {
            m_direction = s_vertex_direction_map.at(direction_name);
        } else {
            return false;
        }

        data["vertices"].get_to(m_vertices);
        data["indices"].get_to(m_indices);

        return true;
    }

    shape::shape(const fs::path& path, bool load) : m_path(path) {
        if (load) {
            if (!reload()) {
                throw std::runtime_error("failed initial load!");
            }
        }
    }
} // namespace sge