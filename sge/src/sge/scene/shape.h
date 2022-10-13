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
    struct shape_vertex {
        glm::vec2 position, uv;
    };

    enum class shape_vertex_direction { clockwise, counter_clockwise };
    struct shape_desc {
        std::vector<shape_vertex> vertices;
        std::vector<std::vector<uint32_t>> indices;
        shape_vertex_direction direction = shape_vertex_direction::clockwise;
    };

    class shape : public asset {
    public:
        static ref<shape> create(const shape_desc& desc, const fs::path& path = fs::path());
        static bool serialize(ref<shape> _shape, const fs::path& path);

        shape(const fs::path& path) : shape(path, true) {}
        virtual ~shape() override = default;

        shape(const shape&) = delete;
        shape& operator=(const shape&) = delete;

        virtual asset_type get_asset_type() override { return asset_type::shape; }
        virtual const fs::path& get_path() override { return m_path; }

        virtual bool reload() override;

    private:
        shape(const fs::path& path, bool load);

        fs::path m_path;

        std::vector<shape_vertex> m_vertices;
        std::vector<std::vector<uint32_t>> m_indices;
        shape_vertex_direction m_direction;
    };
} // namespace sge