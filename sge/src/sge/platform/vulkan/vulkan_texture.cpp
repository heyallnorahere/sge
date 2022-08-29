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
#include "sge/platform/vulkan/vulkan_texture.h"
#include "sge/platform/vulkan/vulkan_context.h"
#include <backends/imgui_impl_vulkan.h>
namespace sge {
    vulkan_texture_2d::vulkan_texture_2d(const texture_spec& spec) {
        m_imgui_id = (ImTextureID) nullptr;
        m_wrap = spec.wrap;
        m_filter = spec.filter;
        m_image = spec.image.as<vulkan_image_2d>();
        m_path = spec.path;
        m_sampler = nullptr;

        VkImageLayout optimal_layout;
        if (m_image->get_usage() & ~image_usage_texture) {
            optimal_layout = VK_IMAGE_LAYOUT_GENERAL;
        } else {
            optimal_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        if (m_image->get_layout() != optimal_layout) {
            m_image->set_layout(optimal_layout);
        }

        if (!create_sampler()) {
            throw std::runtime_error("failed to create sampler!");
        }

        m_image->m_dependents.insert(this);
        m_descriptor_info = vk_init<VkDescriptorImageInfo>();
        m_descriptor_info.imageLayout = m_image->get_layout();
        m_descriptor_info.imageView = m_image->get_view();
        m_descriptor_info.sampler = m_sampler;
    }

    vulkan_texture_2d::~vulkan_texture_2d() {
        if (m_imgui_id != (ImTextureID) nullptr) {
            ImGui_ImplVulkan_RemoveTexture(m_imgui_id);
        }

        m_image->m_dependents.erase(this);

        VkDevice device = vulkan_context::get().get_device().get();
        vkDestroySampler(device, m_sampler, nullptr);
    }

    ImTextureID vulkan_texture_2d::get_imgui_id() {
        if (m_imgui_id == (ImTextureID) nullptr) {
            VkImageView view = m_image->get_view();
            VkImageLayout layout = m_image->get_layout();

            m_imgui_id = ImGui_ImplVulkan_AddTexture(m_sampler, view, layout);
        }

        return m_imgui_id;
    }

    bool vulkan_texture_2d::recreate(ref<image_2d> image, texture_wrap wrap,
                                     texture_filter filter) {
        VkSampler old_sampler = m_sampler;
        texture_wrap old_wrap = m_wrap;
        texture_filter old_filter = m_filter;

        m_wrap = wrap;
        m_filter = filter;

        if (!create_sampler()) {
            m_wrap = old_wrap;
            m_filter = old_filter;

            return false;
        }

        if (m_imgui_id != (ImTextureID) nullptr) {
            VkImageView view = m_image->get_view();
            VkImageLayout layout = m_image->get_layout();

            ImGui_ImplVulkan_UpdateTextureInfo(m_imgui_id, m_sampler, view, layout);
        }

        if (old_sampler != nullptr) {
            auto& device = vulkan_context::get().get_device();
            vkDestroySampler(device.get(), old_sampler, nullptr);
        }

        return true;
    }

    bool vulkan_texture_2d::create_sampler() {
        auto create_info = vk_init<VkSamplerCreateInfo>(VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);

        VkSamplerAddressMode wrap;
        switch (m_wrap) {
        case texture_wrap::clamp:
            wrap = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            break;
        case texture_wrap::repeat:
            wrap = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            break;
        default:
            throw std::runtime_error("invalid texture wrapping mode!");
        }

        create_info.addressModeU = wrap;
        create_info.addressModeV = wrap;
        create_info.addressModeW = wrap;

        VkFilter filter;
        switch (m_filter) {
        case texture_filter::linear:
            filter = VK_FILTER_LINEAR;
            break;
        case texture_filter::nearest:
            filter = VK_FILTER_NEAREST;
            break;
        default:
            throw std::runtime_error("invalid texture filter!");
        }

        create_info.minFilter = filter;
        create_info.magFilter = filter;

        vulkan_device& device = vulkan_context::get().get_device();
        auto physical_device = device.get_physical_device();

        VkPhysicalDeviceFeatures features;
        physical_device.get_features(features);

        if (features.samplerAnisotropy) {
            VkPhysicalDeviceProperties properties;
            physical_device.get_properties(properties);

            create_info.anisotropyEnable = true;
            create_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        } else {
            create_info.anisotropyEnable = false;
            create_info.maxAnisotropy = 1.f;
        }

        create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        create_info.unnormalizedCoordinates = false;

        create_info.compareEnable = false;
        create_info.compareOp = VK_COMPARE_OP_ALWAYS;

        create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        create_info.mipLodBias = 0.f;

        create_info.minLod = 0.f;
        create_info.maxLod = 0.f;

        VkSampler sampler = nullptr;
        if (vkCreateSampler(device.get(), &create_info, nullptr, &sampler) != VK_SUCCESS) {
            return false;
        }

        m_sampler = sampler;
        return true;
    }

    void vulkan_texture_2d::on_layout_transition() {
        VkImageLayout layout = m_image->get_layout();
        m_descriptor_info.imageLayout = layout;

        if (m_imgui_id != (ImTextureID) nullptr) {
            VkImageView view = m_image->get_view();
            ImGui_ImplVulkan_UpdateTextureInfo(m_imgui_id, m_sampler, view, layout);
        }
    }
} // namespace sge