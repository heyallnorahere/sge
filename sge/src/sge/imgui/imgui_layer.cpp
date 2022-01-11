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

        static constexpr float font_size = 16.f;
        io.Fonts->AddFontFromFileTTF("assets/fonts/Roboto-Bold.ttf", font_size);
        io.FontDefault = io.Fonts->AddFontFromFileTTF("assets/fonts/Roboto-Medium.ttf", font_size);

        ImGui::StyleColorsDark();
        set_style();

        if ((io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != ImGuiConfigFlags_None) {
            ImGuiStyle& style = ImGui::GetStyle();
            style.WindowRounding = 0.f;
            style.Colors[ImGuiCol_WindowBg].w = 1.f;
        }

        m_platform = imgui_backend::create_platform_backend();
        m_renderer = imgui_backend::create_renderer_backend();
    }

    void imgui_layer::on_detach() {
        m_renderer.reset();
        m_platform.reset();
        ImGui::DestroyContext();
    }

    void imgui_layer::begin() {
        m_renderer->begin();
        m_platform->begin();
        ImGui::NewFrame();
    }

    void imgui_layer::end(command_list& cmdlist) {
        ImGui::Render();
        m_renderer->render(cmdlist);

        ImGuiIO& io = ImGui::GetIO();
        if ((io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != ImGuiConfigFlags_None) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }

    void imgui_layer::set_style() {
        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text] = ImVec4(0.88f, 0.86f, 0.93f, 0.78f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.88f, 0.86f, 0.93f, 0.28f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.01f, 0.01f, 0.01f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.20f, 0.20f, 0.27f, 0.58f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.20f, 0.20f, 0.27f, 0.90f);
        colors[ImGuiCol_Border] = ImVec4(0.17f, 0.16f, 0.22f, 0.60f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.20f, 0.20f, 0.27f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.27f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.07f, 0.49f, 0.78f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.24f, 0.07f, 0.49f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.17f, 0.05f, 0.36f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.36f, 0.11f, 0.73f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.20f, 0.20f, 0.27f, 0.75f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.20f, 0.20f, 0.27f, 0.47f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.20f, 0.27f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.17f, 0.05f, 0.36f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.24f, 0.07f, 0.49f, 0.78f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.24f, 0.07f, 0.49f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.36f, 0.11f, 0.73f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.27f, 0.37f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.36f, 0.11f, 0.73f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.35f, 0.35f, 0.47f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.07f, 0.49f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.36f, 0.11f, 0.73f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.24f, 0.07f, 0.49f, 0.76f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.24f, 0.07f, 0.49f, 0.86f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.36f, 0.11f, 0.73f, 1.00f);
        colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.47f, 0.77f, 0.83f, 0.04f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.24f, 0.07f, 0.49f, 0.78f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.24f, 0.07f, 0.49f, 1.00f);
        colors[ImGuiCol_Tab] = ImVec4(0.20f, 0.20f, 0.27f, 0.40f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.36f, 0.11f, 0.73f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.24f, 0.07f, 0.49f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.20f, 0.20f, 0.27f, 0.40f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.20f, 0.27f, 0.70f);
        colors[ImGuiCol_DockingPreview] = ImVec4(0.36f, 0.11f, 0.73f, 0.30f);
        colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.88f, 0.86f, 0.93f, 0.63f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.24f, 0.07f, 0.49f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.88f, 0.86f, 0.93f, 0.63f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.24f, 0.07f, 0.49f, 1.00f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.24f, 0.07f, 0.49f, 0.43f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.27f, 0.73f);
    }
} // namespace sge