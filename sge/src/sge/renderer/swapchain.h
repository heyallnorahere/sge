/*
   Copyright 2021 Nora Beda

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
    class swapchain {
    public:
        virtual ~swapchain() = default;

        virtual void on_resize(uint32_t new_width, uint32_t new_height) = 0;

        virtual void new_frame() = 0;
        virtual void present() = 0;

        virtual void begin(ref<command_list> cmdlist, const glm::vec4& clear_color) = 0;
        virtual void end(ref<command_list> cmdlist) = 0;

        virtual size_t get_image_count() = 0;
        virtual uint32_t get_width() = 0;
        virtual uint32_t get_height() = 0;

        virtual size_t get_current_image_index() = 0;
        virtual ref<command_list> get_command_list(size_t index) = 0;
    };
}