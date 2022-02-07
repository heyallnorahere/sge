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

#include "launcher_pch.h"
#include "launcher_layer.h"

namespace sgm::launcher {
    void launcher_layer::on_imgui_render() {
        static constexpr ImGuiConfigFlags required_flags =
            ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;

        ImGuiIO& io = ImGui::GetIO();
        if ((io.ConfigFlags & required_flags) != required_flags) {
            throw std::runtime_error("The SGM launcher cannot be run in this environment.");
        }

        static constexpr ImGuiWindowFlags window_flags =
            ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 1.f));
        ImGui::Begin("Main launcher window", nullptr, window_flags);

        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);

        ImGui::Text("TODO: add controls for launching SGM with projects");
        ImGui::End();
    }
} // namespace sgm::launcher