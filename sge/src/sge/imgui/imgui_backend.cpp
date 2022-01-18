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
#include "sge/imgui/imgui_backend.h"
#ifdef SGE_PLATFORM_DESKTOP
#include "sge/platform/desktop/desktop_imgui_backend.h"
#endif
#ifdef SGE_USE_VULKAN
#include "sge/platform/vulkan/vulkan_base.h"
#include "sge/platform/vulkan/vulkan_imgui_backend.h"
#endif
#ifdef SGE_USE_DIRECTX
#include "sge/platform/directx/directx_base.h"
#include "sge/platform/directx/directx_imgui_backend.h"
#endif
namespace sge {
    std::unique_ptr<imgui_backend> imgui_backend::create_platform_backend() {
        imgui_backend* backend = nullptr;

#ifdef SGE_PLATFORM_DESKTOP
        if (backend == nullptr) {
            backend = new desktop_imgui_backend;
        }
#endif

        return std::unique_ptr<imgui_backend>(backend);
    }

    std::unique_ptr<imgui_backend> imgui_backend::create_renderer_backend() {
        imgui_backend* backend = nullptr;

#ifdef SGE_USE_DIRECTX
        if (backend == nullptr) {
            backend = new directx_imgui_backend;
        }
#endif

#ifdef SGE_USE_VULKAN
        if (backend == nullptr) {
            backend = new vulkan_imgui_backend;
        }
#endif

        return std::unique_ptr<imgui_backend>(backend);
    }
} // namespace sge