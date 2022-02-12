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
#include "sge/core/guid.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace glm {
    void to_json(json& data, const vec2& vec);
    void from_json(const json& data, vec2& vec);
    void to_json(json& data, const vec4& vec);
    void from_json(const json& data, vec4& vec);
} // namespace glm

#ifdef SGE_EXPERIMENTAL_FILESYSTEM
#define SGE_FILESYSTEM_NAMESPACE std::experimental::filesystem
#else
#define SGE_FILESYSTEM_NAMESPACE std::filesystem
#endif

namespace SGE_FILESYSTEM_NAMESPACE {
    void to_json(json& data, const path& path_data);
    void from_json(const json& data, path& path_data);
} // namespace SGE_FILESYSTEM_NAMESPACE

namespace sge {
    void to_json(json& data, const guid& id);
    void from_json(const json& data, guid& id);
} // namespace sge