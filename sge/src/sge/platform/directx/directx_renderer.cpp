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
#include "sge/platform/directx/directx_base.h"
#include "sge/platform/directx/directx_renderer.h"
#include "sge/platform/directx/directx_context.h"
namespace sge {
    void directx_renderer::init() {
        spdlog::warn(
            "the DirectX backend isn't fully implemented yet - crashes may (probably will) occur");

        directx_context::create(D3D_FEATURE_LEVEL_12_0);
    }

    void directx_renderer::shutdown() { directx_context::destroy(); }

    void directx_renderer::wait() {
        // todo: wait for device
    }

    void directx_renderer::submit(const draw_data& data) {
        // todo: draw
    }

    device_info directx_renderer::query_device_info() {
        device_info info;

        // todo: query device

        return info;
    }
} // namespace sge
