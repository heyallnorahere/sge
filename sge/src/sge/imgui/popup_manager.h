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
namespace sge {
    class popup_manager {
    public:
        struct popup_data {
            std::function<void()> callback;
            bool modal = true;

            glm::vec2 size = glm::vec2(0.f, 0.f);
            ImGuiWindowFlags flags = ImGuiWindowFlags_None;
            bool* open_ptr = nullptr;
        };

        popup_manager() = default;

        popup_manager(const popup_manager&) = delete;
        popup_manager& operator=(const popup_manager&) = delete;

        bool open(const std::string& name);
        bool register_popup(const std::string& name, const popup_data& data);

        void update();

    private:
        struct internal_popup_data {
            popup_data data;
            bool opened;
        };

        std::unordered_map<std::string, internal_popup_data> m_data;
    };
} // namespace sge
