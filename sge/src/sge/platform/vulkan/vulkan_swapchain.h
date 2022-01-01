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
#include "sge/renderer/swapchain.h"
#include "sge/core/window.h"
#include "sge/platform/vulkan/vulkan_command_list.h"
namespace sge {
    class vulkan_swapchain : public swapchain {
    public:
        vulkan_swapchain(ref<window> _window);
        virtual ~vulkan_swapchain() override;

        virtual void on_resize(uint32_t new_width, uint32_t new_height) override;

        virtual void new_frame() override;
        virtual void present() override;

        virtual void begin(command_list& cmdlist, const glm::vec4& clear_color) override;
        virtual void end(command_list& cmdlist) override;

        virtual size_t get_image_count() override { return this->m_swapchain_images.size(); }
        virtual uint32_t get_width() override { return this->m_width; }
        virtual uint32_t get_height() override { return this->m_height; }

        virtual size_t get_current_image_index() override { return this->m_current_image_index; }
        virtual command_list& get_command_list(size_t index) override {
            return *this->m_command_buffers[index];
        }

    private:
        bool acquire_next_image();

        void create_render_pass();
        void allocate_command_buffers();
        void create_sync_objects();

        void resize();
        void create(bool render_pass);
        void destroy();

        void create_swapchain();
        void acquire_images();

        static constexpr size_t max_frames_in_flight = 2;

        struct swapchain_image {
            VkImage image;
            VkImageView view;
            VkFramebuffer framebuffer;
        };

        ref<window> m_window;

        uint32_t m_width, m_height;
        VkSurfaceKHR m_surface;
        uint32_t m_present_queue;
        VkSwapchainKHR m_swapchain;
        VkRenderPass m_render_pass;

        VkFormat m_image_format;
        std::vector<swapchain_image> m_swapchain_images;
        uint32_t m_current_image_index;

        struct sync_objects {
            VkSemaphore image_available;
            VkSemaphore render_finished;
            VkFence fence;
        };
        std::array<sync_objects, max_frames_in_flight> m_sync_objects;
        size_t m_current_frame;
        std::vector<VkFence> m_image_fences;

        VkCommandPool m_command_pool;
        std::vector<std::unique_ptr<vulkan_command_list>> m_command_buffers;

        std::optional<glm::uvec2> m_new_size;
    };
} // namespace sge