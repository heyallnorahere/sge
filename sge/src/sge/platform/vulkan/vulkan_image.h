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
#include "sge/renderer/command_list.h"
#include "sge/platform/vulkan/vulkan_buffer.h"
namespace sge {
    VkFormat get_vulkan_image_format(image_format format);
    VkImageUsageFlags get_vulkan_image_usage(uint32_t usage);

    class vulkan_image_2d : public image_2d {
    public:
        vulkan_image_2d(const image_spec& spec);
        virtual ~vulkan_image_2d() override;

        virtual uint32_t get_width() override { return this->m_spec.width; }
        virtual uint32_t get_height() override { return this->m_spec.height; }

        virtual uint32_t get_mip_level_count() override { return this->m_spec.mip_levels; }
        virtual uint32_t get_array_layer_count() override { return this->m_spec.array_layers; }

        virtual image_format get_format() override { return this->m_spec.format; }
        virtual uint32_t get_usage() override { return this->m_spec.image_usage; }

        void set_layout(VkImageLayout new_layout, command_list* cmdlist = nullptr);
        VkImageLayout get_layout() { return this->m_layout; }

        VkImageAspectFlags get_image_aspect() { return this->m_aspect; }
        VkFormat get_vulkan_format() { return this->m_format; }
        VkImageUsageFlags get_image_usage() { return this->m_usage; }
        VkImageView get_view() { return this->m_view; }

    private:
        void create_image();
        void create_view();

        virtual void copy_from(const void* data, size_t size) override;
        void copy_from(ref<vulkan_buffer> source);

        VkImage m_image;
        VkImageView m_view;
        VmaAllocation m_allocation;
        VkFormat m_format;
        VkImageUsageFlags m_usage;
        VkImageLayout m_layout;
        VkImageAspectFlags m_aspect;

        image_spec m_spec;
    };
} // namespace sge