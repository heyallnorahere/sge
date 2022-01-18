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
namespace sge {
    class directx_swapchain_render_pass;
    class directx_swapchain : public swapchain {
    public:
        static constexpr size_t image_count = 3;
        static constexpr DXGI_FORMAT rtv_format = DXGI_FORMAT_R8G8B8A8_UNORM;

        directx_swapchain(ref<window> _window);
        virtual ~directx_swapchain() override;

        virtual void on_resize(uint32_t new_width, uint32_t new_height) override {
            m_resize = true;
        }

        virtual void new_frame() override;
        virtual void present() override;


        virtual ref<render_pass> get_render_pass() override { return m_render_pass; }

        virtual size_t get_image_count() override { return image_count; }
        virtual uint32_t get_width() override { return m_width; }
        virtual uint32_t get_height() override { return m_height; }

        virtual size_t get_current_image_index() override { return m_current_image; }
        virtual command_list& get_command_list(size_t index) override;

    private:
        void begin(command_list& cmdlist, const glm::vec4& clear_color);
        void end(command_list& cmdlist);

        void create_swapchain();
        void create_descriptor_heap();
        void update_rtvs();

        ref<window> m_window;
        uint32_t m_width, m_height;
        bool m_resize;

        ComPtr<IDXGISwapChain4> m_swapchain;
        size_t m_current_image;
        ref<render_pass> m_render_pass;

        ComPtr<ID3D12DescriptorHeap> m_rtv_heap;
        uint32_t m_rtv_size;

        std::array<ComPtr<ID3D12Resource>, image_count> m_backbuffers;
        std::array<command_list*, image_count> m_cmdlists;

        friend class directx_swapchain_render_pass;
    };
} // namespace sge
