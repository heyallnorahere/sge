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
namespace sge {
    enum class texture_wrap { clamp, repeat };
    enum class texture_filter { linear, nearest };

    struct texture_2d_spec {
        ref<image_2d> image;
        texture_wrap wrap = texture_wrap::repeat;
        texture_filter filter = texture_filter::linear;
    };

    class texture_2d : public ref_counted {
    public:
        static ref<texture_2d> create(const texture_2d_spec& spec);

        virtual ~texture_2d() = default;

        virtual ref<image_2d> get_image() = 0;
        virtual texture_wrap get_wrap() = 0;
        virtual texture_filter get_filter() = 0;
    };
} // namespace sge