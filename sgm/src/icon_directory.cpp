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

#include "sgmpch.h"
#include "icon_directory.h"
namespace sgm {
    static std::unordered_map<std::string, ref<texture_2d>> icon_library;

    void icon_directory::load() {
        for (const auto& entry : fs::directory_iterator("assets/icons")) {
            const auto& path = entry.path();

            auto filename = path.filename();
            if (!filename.has_extension()) {
                spdlog::warn("file {0} does not have an extension - skipping", path.string());
                continue;
            }

            auto name = filename.stem().string();
            if (icon_library.find(name) != icon_library.end()) {
                spdlog::warn("icon {0} already exists - skipping", name);
                continue;
            }

            auto img_data = image_data::load(path);
            if (!img_data) {
                spdlog::warn("failed to load {0} - skipping", path.string());
                continue;
            }

            texture_spec spec;
            spec.wrap = texture_wrap::repeat;
            spec.filter = texture_filter::linear;
            spec.image = image_2d::create(img_data, image_usage_none);

            auto icon = texture_2d::create(spec);
            icon_library.insert(std::make_pair(name, icon));
        }
    }

    void icon_directory::clear() { icon_library.clear(); }

    ref<texture_2d> icon_directory::get(const std::string& name) {
        if (icon_library.find(name) == icon_library.end()) {
            return nullptr;
        }

        return icon_library[name];
    }
} // namespace sgm
