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

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <wrl.h>
#include <directx/d3d12.h>
#include <directx/d3dx12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

#ifdef DELETE
#undef DELETE
#endif

namespace sge {
    inline void COM_assert(HRESULT result) {
        if (FAILED(result)) {
            throw std::runtime_error("an error was thrown executing a COM command!");
        }
    }

    inline void create_factory(ComPtr<IDXGIFactory4>& factory) {
        uint32_t flags = 0;

#ifdef SGE_DEBUG
        flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

        COM_assert(CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory)));
    }
} // namespace sge
