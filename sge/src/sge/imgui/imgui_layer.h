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
#include "sge/core/layer.h"
#include "sge/imgui/imgui_backend.h"
#include "sge/renderer/command_list.h"
namespace sge {
    class imgui_layer : public layer {
    public:
        imgui_layer() : layer("ImGui Layer") {}
        virtual ~imgui_layer() override = default;

        imgui_layer(const imgui_layer&) = delete;
        imgui_layer& operator=(const imgui_layer&) = delete;

        virtual void on_attach() override;
        virtual void on_detach() override;

        void begin();
        void end(command_list& cmdlist);

        bool has_font(const fs::path& path);
        ImFont* get_font(const fs::path& path);

    private:
        void set_style();

        std::unique_ptr<imgui_backend> m_platform, m_renderer;
        std::unordered_map<fs::path, ImFont*> m_fonts;
    };
} // namespace sge