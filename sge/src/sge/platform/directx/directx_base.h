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
#include <wrl.h>
#include <directx/d3d12.h>
#include <directx/d3dx12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

namespace sge {
    inline void COM_assert(HRESULT result) {
        if (SUCCEEDED(result)) {
            throw std::runtime_error("an error was thrown executing a COM command!");
        }
    }
} // namespace sge
