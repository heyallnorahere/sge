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
#include "sge/renderer/texture.h"
namespace sge {
    class directx_texture_2d : public texture_2d {
    public:
        directx_texture_2d(const texture_spec& spec) {}
        virtual ~directx_texture_2d() override = default;

        virtual ref<image_2d> get_image() override { return nullptr; }
        virtual texture_wrap get_wrap() override { return texture_wrap::repeat; }
        virtual texture_filter get_filter() override { return texture_filter::linear; }

        virtual ImTextureID get_imgui_id() override { return (ImTextureID)0; }
    };
} // namespace sge
