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
        D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_12_0;
        ComPtr<IDXGIAdapter4> adapter;
        ComPtr<ID3D12Device2> device;
        DWORD info_queue_callback_cookie = 0;
    };

    void directx_context::create(D3D_FEATURE_LEVEL feature_level) {
        if (dx_context) {
            spdlog::warn("attempted to initialize DirectX twice");
            return;
        }

        dx_context = std::unique_ptr<directx_context>(new directx_context);
        dx_context->init(feature_level);
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

    ComPtr<IDXGIAdapter4> directx_context::get_adapter() { return m_data->adapter; }
    ComPtr<ID3D12Device2> directx_context::get_device() { return m_data->device; }

    static void choose_adapter(dx_data* data) {
        ComPtr<IDXGIAdapter1> adapter;

        ComPtr<IDXGIFactory4> factory;
        create_factory(factory);

        size_t max_dedicated_video_memory = 0;
        for (uint32_t i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++) {
            DXGI_ADAPTER_DESC1 adapter_desc;
            adapter->GetDesc1(&adapter_desc);

            if ((adapter_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == DXGI_ADAPTER_FLAG_NONE &&
                SUCCEEDED(D3D12CreateDevice(adapter.Get(), data->feature_level,
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

    static void directx_message_callback(D3D12_MESSAGE_CATEGORY category,
                                         D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID id,
                                         LPCSTR description, void* context) {
        std::string message = "directx info queue: " + std::string(description);

        switch (severity) {
        case D3D12_MESSAGE_SEVERITY_CORRUPTION:
        case D3D12_MESSAGE_SEVERITY_ERROR:
            spdlog::error(message);
            break;
        case D3D12_MESSAGE_SEVERITY_WARNING:
            spdlog::warn(message);
            break;
        default:
            spdlog::info(message);
            break;
        }
    }

    static void create_device(dx_data* data) {
        COM_assert(D3D12CreateDevice(data->adapter.Get(), data->feature_level,
                                     IID_PPV_ARGS(&data->device)));

#ifdef SGE_DEBUG
        ComPtr<ID3D12InfoQueue> info_queue;
        if (SUCCEEDED(data->device.As(&info_queue))) {
            info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);

            static std::vector<D3D12_MESSAGE_ID> deny_ids = {
                // we should allow arbitrary clear values
                D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,

                // the visual studio graphics debugger triggers these
                D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
                D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
            };

            auto filter = dx_init<D3D12_INFO_QUEUE_FILTER>();

            filter.DenyList.NumIDs = deny_ids.size();
            filter.DenyList.pIDList = deny_ids.data();

            COM_assert(info_queue->PushStorageFilter(&filter));

            ComPtr<ID3D12InfoQueue1> info_queue_1;
            if (SUCCEEDED(info_queue.As(&info_queue_1))) {
                COM_assert(info_queue_1->RegisterMessageCallback(
                    directx_message_callback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr,
                    &data->info_queue_callback_cookie));
            } else {
                spdlog::warn("could not register info queue callback!");
            }
        } else {
            spdlog::warn("could not configure the device info queue!");
        }
#endif
    }

    void directx_context::init(D3D_FEATURE_LEVEL feature_level) {
        m_data = new dx_data;
        m_data->feature_level = feature_level;

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
        create_device(m_data);
    }

    void directx_context::shutdown() {
        if (m_data->info_queue_callback_cookie > 0) {
            ComPtr<ID3D12InfoQueue1> info_queue;
            COM_assert(m_data->device.As(&info_queue));
            info_queue->UnregisterMessageCallback(m_data->info_queue_callback_cookie);
        }

        delete m_data;
    }
} // namespace sge
