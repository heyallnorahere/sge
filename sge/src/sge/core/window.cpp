/*
   Copyright 2021 Nora Beda

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
#include "sge/core/window.h"
#include "sge/renderer/renderer.h"
#ifdef SGE_PLATFORM_DESKTOP
#include "sge/platform/desktop/desktop_window.h"
#endif
namespace sge {
    std::unique_ptr<window> window::create(const std::string& title, uint32_t width, uint32_t height) {
        window* instance = nullptr;

#ifdef SGE_PLATFORM_DESKTOP
        instance = new desktop_window(title, width, height);
#endif

        if (instance == nullptr) {
            throw std::runtime_error("no implemented platform was selected!");
        }
        return std::unique_ptr<window>(instance);
    }

    void window::create_swapchain() {
        this->m_swapchain = std::unique_ptr<swapchain>(renderer::create_swapchain(*this));
    }
};