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
#include "sge/platform/directx/directx_base.h"
#include "sge/platform/directx/directx_swapchain.h"
#include "sge/platform/directx/directx_command_list.h"
#include "sge/platform/directx/directx_context.h"
#include "sge/renderer/renderer.h"
namespace sge {
    class directx_swapchain_render_pass : public render_pass {
    public:
        directx_swapchain_render_pass(directx_swapchain* swap_chain) { m_swapchain = swap_chain; }

        virtual void begin(command_list& cmdlist, const glm::vec4& clear_color) override {
            m_swapchain->begin(cmdlist, clear_color);
        }

        virtual void end(command_list& cmdlist) override { m_swapchain->end(cmdlist); }

        virtual render_pass_parent_type get_parent_type() override {
            return render_pass_parent_type::swapchain;
        }

    private:
        directx_swapchain* m_swapchain;
    };

    directx_swapchain::directx_swapchain(ref<window> _window) {
        m_window = _window;
        m_width = m_window->get_width();
        m_height = m_window->get_height();
        m_resize = false;

        for (size_t i = 0; i < image_count; i++) {
            m_cmdlists[i] = nullptr;
        }

        create_swapchain();
        create_descriptor_heap();
        update_rtvs();

        m_render_pass = ref<directx_swapchain_render_pass>::create(this);
    }

    directx_swapchain::~directx_swapchain() {
        auto queue = renderer::get_queue(command_list_type::graphics);
        for (size_t i = 0; i < image_count; i++) {
            if (m_cmdlists[i] != nullptr) {
                queue->submit(*m_cmdlists[i]);
            }
        }
        queue->wait();
    }

    void directx_swapchain::new_frame() {
        // nothing to do
    }

    void directx_swapchain::present() {
        if (m_cmdlists[m_current_image] != nullptr) {
            auto queue = renderer::get_queue(command_list_type::graphics);
            queue->submit(*m_cmdlists[m_current_image], true);

            m_cmdlists[m_current_image] = nullptr;
        }

        // todo: vsync
        COM_assert(m_swapchain->Present(0, 0));
        m_current_image = m_swapchain->GetCurrentBackBufferIndex();

        if (m_resize) {
            m_resize = false;

            m_width = m_window->get_width();
            m_height = m_window->get_height();

            auto queue = renderer::get_queue(command_list_type::graphics);
            queue->wait();

            for (size_t i = 0; i < image_count; i++) {
                m_backbuffers[i].Reset();
            }

            DXGI_SWAP_CHAIN_DESC desc;
            COM_assert(m_swapchain->GetDesc(&desc));
            COM_assert(m_swapchain->ResizeBuffers(image_count, m_width, m_height,
                                                  desc.BufferDesc.Format, desc.Flags));

            m_current_image = m_swapchain->GetCurrentBackBufferIndex();
            update_rtvs();
        }
    }

    command_list& directx_swapchain::get_command_list(size_t index) {
        if (m_cmdlists[index] != nullptr) {
            return *m_cmdlists[index];
        } else {
            auto queue = renderer::get_queue(command_list_type::graphics);
            auto& cmdlist = queue->get();

            m_cmdlists[index] = &cmdlist;
            return cmdlist;
        }
    }

    void directx_swapchain::begin(command_list& cmdlist, const glm::vec4& clear_color) {
        auto dx_cmdlist = (directx_command_list*)&cmdlist;
        auto commandlist = dx_cmdlist->get();

        auto resource = m_backbuffers[m_current_image].Get();
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, D3D12_RESOURCE_STATE_PRESENT,
                                                            D3D12_RESOURCE_STATE_RENDER_TARGET);

        auto rtv = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtv_heap->GetCPUDescriptorHandleForHeapStart(),
                                                 m_current_image, m_rtv_size);

        commandlist->ResourceBarrier(1, &barrier);
        commandlist->ClearRenderTargetView(rtv, &clear_color.x, 0, nullptr);
        commandlist->OMSetRenderTargets(1, &rtv, false, nullptr);
    }

    void directx_swapchain::end(command_list& cmdlist) {
        auto dx_cmdlist = (directx_command_list*)&cmdlist;
        auto commandlist = dx_cmdlist->get();

        auto resource = m_backbuffers[m_current_image].Get();
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            resource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

        commandlist->ResourceBarrier(1, &barrier);
    }

    void directx_swapchain::create_swapchain() {
        auto desc = dx_init<DXGI_SWAP_CHAIN_DESC1>();

        desc.Width = m_width;
        desc.Height = m_height;

        desc.Format = rtv_format;
        desc.Stereo = false;
        desc.SampleDesc = { 1, 0 };
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = image_count;
        desc.Scaling = DXGI_SCALING_STRETCH;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        desc.Flags = 0;

        auto swap_chain = (IDXGISwapChain4*)m_window->create_render_surface(&desc);
        m_swapchain = swap_chain;
        swap_chain->Release();

        m_current_image = m_swapchain->GetCurrentBackBufferIndex();
    }

    void directx_swapchain::create_descriptor_heap() {
        auto desc = dx_init<D3D12_DESCRIPTOR_HEAP_DESC>();

        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = image_count;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask = 0;

        auto device = directx_context::get().get_device();
        COM_assert(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_rtv_heap)));

        m_rtv_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    void directx_swapchain::update_rtvs() {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(m_rtv_heap->GetCPUDescriptorHandleForHeapStart());

        auto device = directx_context::get().get_device();
        for (size_t i = 0; i < image_count; i++) {
            ComPtr<ID3D12Resource> backbuffer;
            COM_assert(m_swapchain->GetBuffer(i, IID_PPV_ARGS(&backbuffer)));
            m_backbuffers[i] = backbuffer;

            device->CreateRenderTargetView(backbuffer.Get(), nullptr, rtv_handle);
            rtv_handle.Offset(m_rtv_size);
        }
    }
} // namespace sge
