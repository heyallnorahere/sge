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
#include "sge/core/application.h"
#include "sge/platform/vulkan/vulkan_base.h"
#include "sge/platform/vulkan/vulkan_imgui_backend.h"
#include "sge/platform/vulkan/vulkan_context.h"
#include "sge/platform/vulkan/vulkan_swapchain.h"
#include "sge/platform/vulkan/vulkan_render_pass.h"
#include "sge/platform/vulkan/vulkan_command_list.h"
#include "sge/renderer/renderer.h"
#include <backends/imgui_impl_vulkan.h>
namespace sge {
    vulkan_imgui_backend::vulkan_imgui_backend() {
        init();
        build_font_atlas();
    }

    vulkan_imgui_backend::~vulkan_imgui_backend() {
        ImGui_ImplVulkan_Shutdown();

        VkDevice device = vulkan_context::get().get_device().get();
        vkDestroyDescriptorPool(device, m_descriptor_pool, nullptr);
    }

    void vulkan_imgui_backend::begin() { ImGui_ImplVulkan_NewFrame(); }
    
    void* vulkan_imgui_backend::render(command_list& cmdlist) {
        auto vk_cmdlist = (vulkan_command_list*)&cmdlist;
        VkCommandBuffer cmdbuffer = vk_cmdlist->get();

        ImDrawData* draw_data = ImGui::GetDrawData();
        ImGui_ImplVulkan_RenderDrawData(draw_data, cmdbuffer);

        return nullptr;
    }

    void vulkan_imgui_backend::init() {
        vulkan_context& context = vulkan_context::get();
        vulkan_device& device = context.get_device();
        VkDevice vk_device = device.get();
        auto physical_device = device.get_physical_device();

        {
            VkDescriptorPoolSize pool_size;
            pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            pool_size.descriptorCount = 1000;

            auto create_info =
                vk_init<VkDescriptorPoolCreateInfo>(VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO);
            create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            create_info.maxSets = 1000;
            create_info.poolSizeCount = 1;
            create_info.pPoolSizes = &pool_size;

            VkResult result =
                vkCreateDescriptorPool(vk_device, &create_info, nullptr, &m_descriptor_pool);
            check_vk_result(result);
        }

        auto& app = application::get();
        auto& swap_chain = (vulkan_swapchain&)app.get_swapchain();
        auto pass = swap_chain.get_render_pass().as<vulkan_render_pass>();

        VkSurfaceCapabilitiesKHR surface_capabilities;
        physical_device.get_surface_capabilities(swap_chain.get_surface(), surface_capabilities);

        vulkan_physical_device::queue_family_indices indices;
        physical_device.query_queue_families(VK_QUEUE_GRAPHICS_BIT, indices);

        auto init_info = vk_init<ImGui_ImplVulkan_InitInfo>();
        init_info.Instance = context.get_instance();
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.PhysicalDevice = physical_device.get();
        init_info.Device = vk_device;
        init_info.MinImageCount = surface_capabilities.minImageCount;
        init_info.ImageCount = swap_chain.get_image_count();
        init_info.QueueFamily = indices.graphics.value();
        init_info.Queue = device.get_queue(init_info.QueueFamily);
        init_info.DescriptorPool = m_descriptor_pool;
        init_info.CheckVkResultFn = check_vk_result;
        ImGui_ImplVulkan_Init(&init_info, pass->get());
    }

    void vulkan_imgui_backend::build_font_atlas() {
        auto queue = renderer::get_queue(command_list_type::transfer);
        auto cmdlist = (vulkan_command_list*)&queue->get();
        cmdlist->begin();

        VkCommandBuffer cmdbuffer = cmdlist->get();
        ImGui_ImplVulkan_CreateFontsTexture(cmdbuffer);

        cmdlist->end();
        queue->submit(*cmdlist, true);

        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }
} // namespace sge