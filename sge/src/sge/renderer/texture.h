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
#include "sge/renderer/image.h"
#include "sge/asset/asset.h"
namespace sge {
    enum class texture_wrap { clamp, repeat };
    enum class texture_filter { linear, nearest };

    struct texture_spec {
        ref<image_2d> image;
        texture_wrap wrap = texture_wrap::repeat;
        texture_filter filter = texture_filter::linear;
        fs::path path;
    };

    class texture_2d : public asset {
    public:
        static ref<texture_2d> create(const texture_spec& spec);
        static ref<texture_2d> load(const fs::path& path);
        static void serialize_settings(ref<texture_2d> texture);

        virtual ~texture_2d() = default;

        virtual ref<image_2d> get_image() = 0;
        virtual texture_wrap get_wrap() = 0;
        virtual texture_filter get_filter() = 0;

        virtual ImTextureID get_imgui_id() = 0;

        virtual asset_type get_asset_type() override { return asset_type::texture_2d; }
    };
} // namespace sge