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
#include "sge/imgui/popup_manager.h"
namespace sge {
    bool popup_manager::open(const std::string& name) {
        if (m_data.find(name) != m_data.end()) {
            m_data[name].opened = true;
            return true;
        }

        return false;
    }

    bool popup_manager::register_popup(const std::string& name, const popup_data& data) {
        if (m_data.find(name) != m_data.end()) {
            return false;
        }

        if (!data.callback) {
            return false;
        }

        internal_popup_data popup;
        popup.data = data;
        popup.opened = false;

        m_data.insert(std::make_pair(name, popup));
        return true;
    }

    void popup_manager::update() {
        for (auto& [name, popup] : m_data) {
            const char* id = name.c_str();
            if (popup.opened) {
                ImGui::OpenPopup(id);
                popup.opened = false;
            }

            bool* p_open = popup.data.open_ptr;
            if (popup.data.modal && p_open != nullptr && !*p_open) {
                continue;
            }

            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

            glm::vec2 size = popup.data.size;
            ImGui::SetNextWindowSize(ImVec2(size.x, size.y), ImGuiCond_Appearing);

            static constexpr ImGuiWindowFlags popup_flags =
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoDocking;
            ImGuiWindowFlags flags = popup_flags | popup.data.flags;

            bool open;
            if (popup.data.modal) {
                open = ImGui::BeginPopupModal(id, p_open, flags);
            } else {
                open = ImGui::BeginPopup(id, flags);
            }

            if (open) {
                popup.data.callback();
                ImGui::EndPopup();
            }
        }
    }
} // namespace sge
