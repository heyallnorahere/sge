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
#include "sge/renderer/command_queue.h"
#include "sge/platform/directx/directx_command_list.h"
namespace sge {
    class directx_command_queue : public command_queue {
    public:
        static D3D12_COMMAND_LIST_TYPE get_cmdlist_type(command_list_type type);

        directx_command_queue(command_list_type type);
        virtual ~directx_command_queue() override;

        virtual void wait() override;
        uint64_t signal();
        bool is_fence_complete(uint64_t value);
        void wait_for_fence_value(uint64_t value);

        virtual command_list& get() override;
        virtual void submit(command_list& cmdlist, bool wait = false) override;

        virtual command_list_type get_type() override { return m_type; }

    private:
        directx_command_list* create_cmdlist();

        struct command_list_entry {
            uint64_t fence_value;
            std::unique_ptr<directx_command_list> cmdlist;
        };

        command_list_type m_type;
        ComPtr<ID3D12CommandQueue> m_queue;

        ComPtr<ID3D12Fence> m_fence;
        HANDLE m_fence_event;
        uint64_t m_fence_value;

        std::queue<command_list_entry> m_cmdlists;
    };
} // namespace sge
