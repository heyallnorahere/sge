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
#include "sge/platform/directx/directx_command_list.h"
namespace sge {
    directx_command_list::directx_command_list(ComPtr<ID3D12CommandAllocator> allocator,
                                               ComPtr<ID3D12GraphicsCommandList2> cmdlist) {
        m_cmdlist = cmdlist;
        m_allocator = allocator;
    }

    void directx_command_list::reset() {
        COM_assert(m_allocator->Reset());
        COM_assert(m_cmdlist->Reset(m_allocator.Get(), nullptr));
    }

    void directx_command_list::begin() {
        // nothing
    }

    void directx_command_list::end() { COM_assert(m_cmdlist->Close()); }
} // namespace sge