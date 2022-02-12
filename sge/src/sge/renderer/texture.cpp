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
#include "sge/renderer/texture.h"
#include "sge/asset/json.h"
#ifdef SGE_USE_VULKAN
#include "sge/platform/vulkan/vulkan_base.h"
#include "sge/platform/vulkan/vulkan_texture.h"
#endif
#ifdef SGE_USE_DIRECTX
#include "sge/platform/directx/directx_base.h"
#include "sge/platform/directx/directx_texture.h"
#endif
namespace sge {
    ref<texture_2d> texture_2d::create(const texture_spec& spec) {
        if (!spec.image) {
            throw std::runtime_error("cannot create a texture from nullptr!");
        } else if ((spec.image->get_usage() & image_usage_texture) == 0) {
            throw std::runtime_error("cannot create a texture from the passed image!");
        }

#ifdef SGE_USE_DIRECTX
        return ref<directx_texture_2d>::create(spec);
#endif

#ifdef SGE_USE_VULKAN
        return ref<vulkan_texture_2d>::create(spec);
#endif

        return nullptr;
    }

    ref<texture_2d> texture_2d::load(const fs::path& path) {
        if (!fs::exists(path)) {
            return nullptr;
        }

        auto img_data = image_data::load(path);
        if (!img_data) {
            return nullptr;
        }

        texture_spec spec;
        spec.path = path;
        spec.image = image_2d::create(img_data, image_usage_none);

        fs::path settings_path = path.string() + ".sgetexture";
        if (fs::exists(settings_path)) {
            json data;

            std::ifstream stream(settings_path);
            stream >> data;
            stream.close();

            json wrap_data = data["wrap"];
            if (!wrap_data.is_null()) {
                std::string wrap_string = wrap_data.get<std::string>();

                if (wrap_string == "clamp") {
                    spec.wrap = texture_wrap::clamp;
                } else if (wrap_string == "repeat") {
                    spec.wrap = texture_wrap::repeat;
                } else {
                    throw std::runtime_error("invalid wrap: " + wrap_string);
                }
            }

            json filter_data = data["filter"];
            if (!filter_data.is_null()) {
                std::string filter_string = filter_data.get<std::string>();

                if (filter_string == "linear") {
                    spec.filter = texture_filter::linear;
                } else if (filter_string == "repeat") {
                    spec.filter = texture_filter::nearest;
                } else {
                    throw std::runtime_error("invalid filter: " + filter_string);
                }
            }
        }

        return create(spec);
    }

    void texture_2d::serialize_settings(ref<texture_2d> texture, const fs::path& path) {
        if (path.empty()) {
            spdlog::warn("attempted to serialize to a nonexistent path!");
            return;
        }

        json data;

        std::string wrap_string;
        switch (texture->get_wrap()) {
        case texture_wrap::clamp:
            wrap_string = "clamp";
            break;
        case texture_wrap::repeat:
            wrap_string = "repeat";
            break;
        default:
            throw std::runtime_error("invalid texture wrap!");
        }
        data["wrap"] = wrap_string;

        std::string filter_string;
        switch (texture->get_filter()) {
        case texture_filter::linear:
            filter_string = "linear";
            break;
        case texture_filter::nearest:
            filter_string = "nearest";
            break;
        default:
            throw std::runtime_error("invalid texture filter!");
        }
        data["filter"] = filter_string;

        fs::path settings_path = path.string() + ".sgetexture";
        std::ofstream stream(settings_path);
        stream << data.dump(4) << std::flush;
        stream.close();
    }
} // namespace sge