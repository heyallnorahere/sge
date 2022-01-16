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
#include "sge/platform/directx/directx_context.h"
namespace sge {
    static std::unique_ptr<directx_context> dx_context;

    struct dx_data {
        // todo: directx device
        uint32_t unused = 0;
    };

    void directx_context::create() {
        if (dx_context) {
            spdlog::warn("attempted to initialize DirectX twice");
            return;
        }

        dx_context = std::unique_ptr<directx_context>(new directx_context);
        dx_context->init();
    }

    void directx_context::destroy() {
        if (!dx_context) {
            spdlog::warn("attempted to destroy a nonexistent DirectX context");
            return;
        }

        dx_context->shutdown();
        dx_context.reset();
    }

    directx_context& directx_context::get() { return *dx_context; }

    void directx_context::init() {
        m_data = new dx_data;

#ifdef SGE_DEBUG
        {
            ComPtr<ID3D12Debug> debug_interface;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface)))) {
                debug_interface->EnableDebugLayer();
            } else {
                spdlog::warn("could not enable the DirectX debug layer");
            }
        }
#endif

        // todo: create device
    }

    void directx_context::shutdown() {
        // todo: destroy device

        delete m_data;
    }
} // namespace sge
