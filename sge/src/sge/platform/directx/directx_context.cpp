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
        ComPtr<IDXGIAdapter4> adapter;
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

    static void choose_adapter(dx_data* data) {
        ComPtr<IDXGIAdapter1> adapter;

        ComPtr<IDXGIFactory4> factory;
        create_factory(factory);

        size_t max_dedicated_video_memory = 0;
        for (uint32_t i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++) {
            DXGI_ADAPTER_DESC1 adapter_desc;
            adapter->GetDesc1(&adapter_desc);

            if ((adapter_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == DXGI_ADAPTER_FLAG_NONE &&
                SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0,
                                            __uuidof(ID3D12Device), nullptr)) &&
                adapter_desc.DedicatedVideoMemory > max_dedicated_video_memory) {
                if (SUCCEEDED(adapter.As(&data->adapter))) {
                    max_dedicated_video_memory = adapter_desc.DedicatedVideoMemory;
                }
            }
        }

        if (data->adapter) {
            DXGI_ADAPTER_DESC adapter_desc;
            data->adapter->GetDesc(&adapter_desc);

            std::wstring wchar_name = adapter_desc.Description;
            std::string name;

            // hack hacky hack hack
            for (wchar_t character : wchar_name) {
                name.push_back((char)character);
            }

            spdlog::info("chose adapter: {0}", name);
        } else {
            throw std::runtime_error("could not find a suitable adapter!");
        }
    }

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

        choose_adapter(m_data);
        // todo: create device
    }

    void directx_context::shutdown() {
        // todo: destroy device

        delete m_data;
    }
} // namespace sge
