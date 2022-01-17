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
#include "sge/platform/directx/directx_command_queue.h"
#include "sge/platform/directx/directx_context.h"
namespace sge {
    D3D12_COMMAND_LIST_TYPE directx_command_queue::get_cmdlist_type(command_list_type type) {
        switch (type) {
        case command_list_type::graphics:
            return D3D12_COMMAND_LIST_TYPE_DIRECT;
        case command_list_type::compute:
            return D3D12_COMMAND_LIST_TYPE_COMPUTE;
        case command_list_type::transfer:
            return D3D12_COMMAND_LIST_TYPE_COPY;
        default:
            throw std::runtime_error("invalid command list type!");
            return D3D12_COMMAND_LIST_TYPE_DIRECT;
        }
    }

    directx_command_queue::directx_command_queue(command_list_type type) {
        m_fence_value = 0;
        m_type = type;

        auto desc = dx_init<D3D12_COMMAND_QUEUE_DESC>();
        desc.Type = get_cmdlist_type(m_type);
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

        auto device = directx_context::get().get_device();
        COM_assert(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_queue)));
        COM_assert(
            device->CreateFence(m_fence_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));

        m_fence_event = ::CreateEvent(nullptr, false, false, nullptr);
        if (m_fence_event == nullptr) {
            throw std::runtime_error("could not create fence!");
        }
    }

    directx_command_queue::~directx_command_queue() { ::CloseHandle(m_fence_event); }

    void directx_command_queue::wait() {
        uint64_t fence_value = signal();
        wait_for_fence_value(fence_value);
    }

    uint64_t directx_command_queue::signal() {
        uint64_t fence_value = ++m_fence_value;
        m_queue->Signal(m_fence.Get(), fence_value);
        return fence_value;
    }

    bool directx_command_queue::is_fence_complete(uint64_t value) {
        return m_fence->GetCompletedValue() >= value;
    }

    void directx_command_queue::wait_for_fence_value(uint64_t value) {
        if (!is_fence_complete(value)) {
            m_fence->SetEventOnCompletion(value, m_fence_event);
            ::WaitForSingleObject(m_fence_event, std::numeric_limits<DWORD>::max());
        }
    }

    command_list& directx_command_queue::get() {
        directx_command_list* cmdlist;

        if (!m_cmdlists.empty() && is_fence_complete(m_cmdlists.front().fence_value)) {
            cmdlist = m_cmdlists.front().cmdlist.release();
            m_cmdlists.pop();

            cmdlist->reset();
        } else {
            cmdlist = create_cmdlist();
        }

        return *cmdlist;
    }

    void directx_command_queue::submit(command_list& cmdlist, bool wait) {
        auto dx_cmdlist = (directx_command_list*)&cmdlist;
        auto commandlist = dx_cmdlist->get();

        ID3D12CommandList* raw = commandlist.Get();
        m_queue->ExecuteCommandLists(1, &raw);

        uint64_t fence_value = signal();
        m_cmdlists.emplace<command_list_entry>(
            { fence_value, std::unique_ptr<directx_command_list>(dx_cmdlist) });

        if (wait) {
            wait_for_fence_value(fence_value);
        }
    }

    directx_command_list* directx_command_queue::create_cmdlist() {
        auto device = directx_context::get().get_device();
        auto type = get_cmdlist_type(m_type);

        ComPtr<ID3D12CommandAllocator> allocator;
        COM_assert(device->CreateCommandAllocator(type, IID_PPV_ARGS(&allocator)));

        ComPtr<ID3D12GraphicsCommandList2> cmdlist;
        COM_assert(
            device->CreateCommandList(0, type, allocator.Get(), nullptr, IID_PPV_ARGS(&cmdlist)));

        return new directx_command_list(allocator, cmdlist);
    }
} // namespace sge
