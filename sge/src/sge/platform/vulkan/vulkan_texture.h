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
#include "sge/platform/vulkan/vulkan_image.h"
namespace sge {
    class vulkan_texture_2d : public texture_2d {
    public:
        vulkan_texture_2d(const texture_spec& spec);
        virtual ~vulkan_texture_2d() override;

        virtual void reload() override;

        virtual ref<image_2d> get_image() override { return m_image; }
        virtual texture_wrap get_wrap() override { return m_wrap; }
        virtual texture_filter get_filter() override { return m_filter; }
        virtual const fs::path& get_path() override { return m_path; }

        virtual ImTextureID get_imgui_id() override;

        const VkDescriptorImageInfo& get_descriptor_info() { return m_descriptor_info; }

    private:
        void create_sampler();
        void on_layout_transition();

        VkSampler m_sampler;
        ref<vulkan_image_2d> m_image;

        texture_wrap m_wrap;
        texture_filter m_filter;
        fs::path m_path;

        VkDescriptorImageInfo m_descriptor_info;
        ImTextureID m_imgui_id;

        friend class vulkan_image_2d;
    };
} // namespace sge