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
#include "sge/asset/json.h"
namespace glm {
    void to_json(json& data, const vec2& vec) {
        for (length_t i = 0; i < 2; i++) {
            data.push_back(vec[i]);
        }
    }

    void from_json(const json& data, vec2& vec) {
        if (data.size() != 2) {
            throw std::runtime_error("malformed json");
        }

        for (length_t i = 0; i < 2; i++) {
            vec[i] = data[i].get<float>();
        }
    }

    void to_json(json& data, const vec4& vec) {
        for (length_t i = 0; i < 4; i++) {
            data.push_back(vec[i]);
        }
    }

    void from_json(const json& data, vec4& vec) {
        if (data.size() != 4) {
            throw std::runtime_error("malformed json");
        }

        for (length_t i = 0; i < 4; i++) {
            vec[i] = data[i].get<float>();
        }
    }
} // namespace glm

namespace SGE_FILESYSTEM_NAMESPACE {
    void to_json(json& data, const path& path_data) {
        std::string result = path_data.string();

        size_t pos;
        while ((pos = result.find('\\')) != std::string::npos) {
            result.replace(pos, 1, "/");
        }

        data = result;
    }

    void from_json(const json& data, path& path_data) {
        path raw_path = data.get<std::string>();
        path_data = raw_path.make_preferred();
    }
} // namespace SGE_FILESYSTEM_NAMESPACE

namespace sge {
    void to_json(json& data, const guid& id) { data = (uint64_t)id; }
    void from_json(const json& data, guid& id) { id = data.get<uint64_t>(); }
} // namespace sge