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
#include "sge/imgui/imgui_layer.h"
namespace sge {
    void imgui_layer::on_attach() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

#ifdef SGE_PLATFORM_DESKTOP
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif

        // todo: type faces

        ImGui::StyleColorsDark();
        if ((io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != ImGuiConfigFlags_None) {
            ImGuiStyle& style = ImGui::GetStyle();
            style.WindowRounding = 0.f;
            style.Colors[ImGuiCol_WindowBg].w = 1.f;
        }

        // todo: further styles
        // https://github.com/ocornut/imgui/issues/2265#issuecomment-465432091

        this->m_platform = imgui_backend::create_platform_backend();
        this->m_renderer = imgui_backend::create_renderer_backend();
    }

    void imgui_layer::on_detach() {
        this->m_renderer.reset();
        this->m_platform.reset();
        ImGui::DestroyContext();
    }

    void imgui_layer::begin() {
        this->m_renderer->begin();
        this->m_platform->begin();
        ImGui::NewFrame();
    }

    void imgui_layer::end(command_list& cmdlist) {
        ImGui::Render();
        this->m_renderer->render(cmdlist);

        ImGuiIO& io = ImGui::GetIO();
        if ((io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != ImGuiConfigFlags_None) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }
} // namespace sge