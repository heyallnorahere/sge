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
#include <cstdint>
#include <stddef.h>

#include <string>
#include <sstream>
#include <vector>
#include <array>
#include <fstream>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <utility>
#include <functional>
#include <memory>
#include <queue>
#include <stack>
#include <limits>
#include <optional>
#include <thread>
#include <chrono>
#include <mutex>
#include <cassert>

#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#define SGE_EXPERIMENTAL_FILESYSTEM
#endif

#include <spdlog/spdlog.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include "sge/ref.h"

namespace std {
    template <>
    struct hash<fs::path> {
        size_t operator()(const fs::path& path) const {
            hash<string> hasher;
            return hasher(path.string());
        }
    };
} // namespace std

namespace sge {
    using timestep = std::chrono::duration<double>;
}