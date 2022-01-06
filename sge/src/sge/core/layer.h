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
#include "sge/events/event.h"
namespace sge {
    class layer {
    public:
        layer(const std::string& name = "Layer") { this->m_name = name; }
        virtual ~layer() = default;

        virtual void on_attach() {}
        virtual void on_detach() {}

        virtual void on_update(timestep ts) {}
        virtual void on_imgui_render() {}
        virtual void on_event(event& e) {}

        const std::string& get_name() const { return this->m_name; }

    protected:
        std::string m_name;
    };
}; // namespace sge