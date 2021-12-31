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
#include "sge/core/window.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
namespace sge {
    class desktop_window : public window {
    public:
        desktop_window(const std::string& title, uint32_t width, uint32_t height);
        virtual ~desktop_window() override;

        virtual void on_update() override;

        virtual uint32_t get_width() override { return this->m_data.width; }
        virtual uint32_t get_height() override { return this->m_data.height; }

        virtual void set_event_callback(event_callback_t callback) override;

        virtual void* get_native_window() override { return this->m_window; }
        virtual void* create_render_surface(void* params) override;
        virtual void get_vulkan_extensions(std::set<std::string>& extensions) override;
    
    private:
        void setup_event_callbacks();

        GLFWwindow* m_window;

        struct window_data {
            std::string title;
            uint32_t width, height;
            event_callback_t event_callback;
        };
        window_data m_data;
    };
}