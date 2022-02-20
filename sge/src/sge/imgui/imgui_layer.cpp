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

// generated in build/sge/type_face_directory.cpp by tools/embed_type_faces.cpp
extern std::unordered_map<std::string, std::vector<uint32_t>> generated_type_face_directory;

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

        for (const auto& [key, data] : generated_type_face_directory) {
            // memory will be freed by ImGui
            size_t size = data.size() * sizeof(uint32_t);
            void* font_data = malloc(size);

            if (font_data == nullptr) {
                throw std::runtime_error("could not allocate memory to load font: " + key);
            }

            float font_size = 16.f;
            if (key.find("Bold") != std::string::npos) {
                font_size = 32.f;
            }

            memcpy(font_data, data.data(), size);
            ImFont* font = io.Fonts->AddFontFromMemoryTTF(font_data, (int32_t)size, font_size);

            fs::path path = key;
            if (m_fonts.find(path) == m_fonts.end()) {
                m_fonts.insert(std::make_pair(path, font));
            } else {
                spdlog::warn("font \"{0}}\" already present - replacing", key);
                m_fonts[path] = font;
            }
        }

        fs::path default_font = "Roboto-Medium.ttf";
        if (has_font(default_font)) {
            io.FontDefault = get_font(default_font);
        } else {
            io.Fonts->AddFontDefault();
        }

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

    bool imgui_layer::has_font(const fs::path& path) { return m_fonts.find(path) != m_fonts.end(); }

    ImFont* imgui_layer::get_font(const fs::path& path) {
        ImFont* font = nullptr;

        if (has_font(path)) {
            font = m_fonts[path];
        }

        return font;
    }

    void imgui_layer::set_style() {
        // from https://github.com/TheCherno/Hazel/blob/master/Hazel/src/Hazel/ImGui/ImGuiLayer.cpp
        // todo: write our own style, though this is good enough FOR NOW

		auto& colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.105f, 0.11f, 1.f);
		colors[ImGuiCol_Header] = ImVec4(0.2f, 0.205f, 0.21f, 1.f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.3f, 0.305f, 0.31f, 1.f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.15f, 0.1505f, 0.151f, 1.f);
		
		colors[ImGuiCol_Button] = ImVec4(0.2f, 0.205f, 0.21f, 1.f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.3f, 0.305f, 0.31f, 1.f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.1505f, 0.151f, 1.f);

		colors[ImGuiCol_FrameBg] = ImVec4(0.2f, 0.205f, 0.21f, 1.f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.3f, 0.305f, 0.31f, 1.f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.15f, 0.1505f, 0.151f, 1.f);

		colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.1505f, 0.151f, 1.f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.38f, 0.3805f, 0.381f, 1.f);
		colors[ImGuiCol_TabActive] = ImVec4(0.28f, 0.2805f, 0.281f, 1.f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.1505f, 0.151f, 1.f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.2f, 0.205f, 0.21f, 1.f);

		colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.1505f, 0.151f, 1.f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.1505f, 0.151f, 1.f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.15f, 0.1505f, 0.151f, 1.f);
    }
} // namespace sge