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
} // namespace sge