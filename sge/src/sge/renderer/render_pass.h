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
#include "sge/renderer/command_list.h"
namespace sge {
    enum class render_pass_parent_type {
        swapchain,
        framebuffer
    };

    class render_pass : public ref_counted {
    public:
        virtual ~render_pass() = default;

        virtual render_pass_parent_type get_parent_type() = 0;

        virtual void begin(command_list& cmdlist, const glm::vec4& clear_color) = 0;
        virtual void end(command_list& cmdlist) = 0;
    };
} // namespace sge