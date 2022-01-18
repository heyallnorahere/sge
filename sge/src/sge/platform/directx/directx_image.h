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
    class directx_image_2d : public image_2d {
    public:
        directx_image_2d(const image_spec& spec) {}
        virtual ~directx_image_2d() override = default;

        virtual uint32_t get_width() override { return 0; }
        virtual uint32_t get_height() override { return 0; }

        virtual uint32_t get_mip_level_count() override { return 0; }
        virtual uint32_t get_array_layer_count() override { return 0; }

        virtual image_format get_format() override { return image_format::RGBA8_UINT; }
        virtual uint32_t get_usage() override { return image_usage_texture; }

    protected:
        virtual void copy_from(const void* data, size_t size) override {}
    };
} // namespace sge
