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
    class window : public ref_counted {
    public:
        static ref<window> create(const std::string& title, uint32_t width, uint32_t height);

        using event_callback_t = std::function<void(event&)>;

        virtual ~window() = default;

        virtual void on_update() = 0;

        virtual uint32_t get_width() = 0;
        virtual uint32_t get_height() = 0;

        virtual void set_event_callback(event_callback_t callback) = 0;

        virtual void* get_native_window() = 0;
        virtual void* create_render_surface(void* params) = 0;
        virtual void get_vulkan_extensions(std::set<std::string>& extensions) = 0;
    };
}; // namespace sge