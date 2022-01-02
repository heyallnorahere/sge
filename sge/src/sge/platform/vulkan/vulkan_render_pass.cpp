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
#include "sge/platform/vulkan/vulkan_base.h"
#include "sge/platform/vulkan/vulkan_render_pass.h"
#include "sge/platform/vulkan/vulkan_context.h"
namespace sge {
    vulkan_render_pass::vulkan_render_pass(VkRenderPass vk_render_pass) {
        this->m_render_pass = vk_render_pass;
    }

    vulkan_render_pass::~vulkan_render_pass() {
        VkDevice device = vulkan_context::get().get_device().get();
        vkDestroyRenderPass(device, this->m_render_pass, nullptr);
    }
} // namespace sge