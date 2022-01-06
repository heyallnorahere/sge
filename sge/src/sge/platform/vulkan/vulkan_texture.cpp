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
    vulkan_texture_2d::vulkan_texture_2d(const texture_2d_spec& spec) {
        this->m_imgui_id = (ImTextureID)nullptr;
        this->m_wrap = spec.wrap;
        this->m_filter = spec.filter;
        this->m_image = spec.image.as<vulkan_image_2d>();

        VkImageLayout optimal_layout;
        if (this->m_image->get_usage() & ~image_usage_texture) {
            optimal_layout = VK_IMAGE_LAYOUT_GENERAL;
        } else {
            optimal_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }

        if (this->m_image->get_layout() != optimal_layout) {
            this->m_image->set_layout(optimal_layout);
        }

        this->create_sampler();

        this->m_image->m_dependents.insert(this);
        this->m_descriptor_info = vk_init<VkDescriptorImageInfo>();
        this->m_descriptor_info.imageLayout = this->m_image->get_layout();
        this->m_descriptor_info.imageView = this->m_image->get_view();
        this->m_descriptor_info.sampler = this->m_sampler;
    }

    vulkan_texture_2d::~vulkan_texture_2d() {
        if (this->m_imgui_id != (ImTextureID)nullptr) {
            ImGui_ImplVulkan_RemoveTexture(this->m_imgui_id);
        }

        this->m_image->m_dependents.erase(this);

        VkDevice device = vulkan_context::get().get_device().get();
        vkDestroySampler(device, this->m_sampler, nullptr);
    }

    ImTextureID vulkan_texture_2d::get_imgui_id() {
        if (this->m_imgui_id == (ImTextureID)nullptr) {
            VkImageView view = this->m_image->get_view();
            VkImageLayout layout = this->m_image->get_layout();
            this->m_imgui_id = ImGui_ImplVulkan_AddTexture(this->m_sampler, view, layout);
        }

        return this->m_imgui_id;
    }

    void vulkan_texture_2d::create_sampler() {
        auto create_info = vk_init<VkSamplerCreateInfo>(VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);
        
        VkSamplerAddressMode wrap;
        switch (this->m_wrap) {
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
        switch (this->m_filter) {
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

        VkResult result = vkCreateSampler(device.get(), &create_info, nullptr, &this->m_sampler);
        check_vk_result(result);
    }

    void vulkan_texture_2d::on_layout_transition() {
        VkImageLayout layout = this->m_image->get_layout();
        this->m_descriptor_info.imageLayout = layout;

        if (this->m_imgui_id != (ImTextureID)nullptr) {
            VkImageView view = this->m_image->get_view();
            ImGui_ImplVulkan_UpdateTextureInfo(this->m_imgui_id, this->m_sampler, view, layout);
        }
    }
} // namespace sge