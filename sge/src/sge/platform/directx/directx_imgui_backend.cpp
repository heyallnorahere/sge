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
#include "sge/platform/directx/directx_imgui_backend.h"
#include "sge/platform/directx/directx_context.h"
#include "sge/platform/directx/directx_swapchain.h"
#include "sge/platform/directx/directx_command_list.h"
#include <backends/imgui_impl_dx12.h>
namespace sge {
    directx_imgui_backend::directx_imgui_backend() {
        auto device = directx_context::get().get_device();

        {
            auto desc = dx_init<D3D12_DESCRIPTOR_HEAP_DESC>();

            desc.NumDescriptors = 1;
            desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            desc.NodeMask = 0;
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

            COM_assert(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_srv_heap)));
        }

        ImGui_ImplDX12_Init(device.Get(), directx_swapchain::image_count,
                            directx_swapchain::rtv_format, m_srv_heap.Get(),
                            m_srv_heap->GetCPUDescriptorHandleForHeapStart(),
                            m_srv_heap->GetGPUDescriptorHandleForHeapStart());
    }

    directx_imgui_backend::~directx_imgui_backend() { ImGui_ImplDX12_Shutdown(); }
    void directx_imgui_backend::begin() { ImGui_ImplDX12_NewFrame(); }

    void* directx_imgui_backend::render(command_list& cmdlist) {
        auto dx_cmdlist = (directx_command_list*)&cmdlist;
        auto commandlist = dx_cmdlist->get();

        ID3D12DescriptorHeap* heap = m_srv_heap.Get();
        commandlist->SetDescriptorHeaps(1, &heap);

        auto raw_cmdlist = commandlist.Get();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), raw_cmdlist);

        return (void*)raw_cmdlist;
    }
} // namespace sge
