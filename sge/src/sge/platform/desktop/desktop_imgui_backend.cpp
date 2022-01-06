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
#include "sge/platform/desktop/desktop_imgui_backend.h"
#include "sge/platform/desktop/desktop_window.h"
#include "sge/core/application.h"
#include <backends/imgui_impl_glfw.h>
namespace sge {
    desktop_imgui_backend::desktop_imgui_backend() {
        bool (*init_func)(GLFWwindow * glfw_window, bool install_callbacks);

#ifdef SGE_USE_VULKAN
        init_func = ImGui_ImplGlfw_InitForVulkan;
#else
        init_func = ImGui_ImplGlfw_InitForOther;
#endif

        auto _window = application::get().get_window();
        auto glfw_window = (GLFWwindow*)_window->get_native_window();
        init_func(glfw_window, true);
    }

    desktop_imgui_backend::~desktop_imgui_backend() { ImGui_ImplGlfw_Shutdown(); }
    void desktop_imgui_backend::begin() { ImGui_ImplGlfw_NewFrame(); }
} // namespace sge