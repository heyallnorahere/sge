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

#include "sgepch.h"
#ifdef SGE_USE_VULKAN
#include "sge/platform/vulkan/vulkan_base.h"
#endif
#include "sge/platform/desktop/desktop_window.h"
#include "sge/events/window_events.h"
namespace sge {
    static uint32_t glfw_window_count = 0;

    desktop_window::desktop_window(const std::string& title, uint32_t width, uint32_t height) {
        this->m_data.title = title;
        this->m_data.width = width;
        this->m_data.height = height;
        this->m_data.event_callback = nullptr;
        spdlog::info("creating window:\n\ttitle: {0}\n\tsize: ({1}, {2})", title, width, height);

        if (glfw_window_count == 0) {
            if (!glfwInit()) {
                throw std::runtime_error("could not initialize glfw!");
            }
            glfwSetErrorCallback([](int32_t code, const char* description) {
                spdlog::error("glfw error: {0} ({1})", description, code);
            });
        }

        // we are not going to support OpenGL
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        this->m_window = glfwCreateWindow(this->m_data.width, this->m_data.height, this->m_data.title.c_str(), nullptr, nullptr);
        if (this->m_window == nullptr) {
            throw std::runtime_error("could not create glfw window!");
        }

        this->setup_event_callbacks();
    }

    desktop_window::~desktop_window() {
        glfwDestroyWindow(this->m_window);
        glfw_window_count--;

        if (glfw_window_count == 0) {
            glfwTerminate();
        }
    }

    void desktop_window::on_update() {
        glfwPollEvents();
    }

    void desktop_window::set_event_callback(event_callback_t callback) {
        this->m_data.event_callback = callback;
    }

    void* desktop_window::create_render_surface(void* params) {
#ifdef SGE_USE_VULKAN
        {
            auto instance = (VkInstance)params;

            VkSurfaceKHR surface;
            VkResult result = glfwCreateWindowSurface(
                instance, this->m_window, nullptr, &surface);
            check_vk_result(result);

            return surface;
        }
#endif

        return nullptr;
    }

    void desktop_window::get_vulkan_extensions(std::set<std::string>& extensions) {
#ifdef SGE_USE_VULKAN
        uint32_t glfw_extension_count = 0;
        const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

        for (uint32_t i = 0; i < glfw_extension_count; i++) {
            extensions.insert(glfw_extensions[i]);
        }
#endif
    }

    void desktop_window::setup_event_callbacks() {
        glfwSetWindowUserPointer(this->m_window, &this->m_data);

        glfwSetWindowSizeCallback(this->m_window, [](GLFWwindow* window, int32_t width, int32_t height) {
            window_data* wd = (window_data*)glfwGetWindowUserPointer(window);
            wd->width = (uint32_t)width;
            wd->height = (uint32_t)height;

            if (wd->event_callback != nullptr) {
                window_resize_event resize_event(wd->width, wd->height);
                wd->event_callback(resize_event);
            }
        });

        glfwSetWindowCloseCallback(this->m_window, [](GLFWwindow* window) {
            window_data* wd = (window_data*)glfwGetWindowUserPointer(window);

            if (wd->event_callback != nullptr) {
                window_close_event close_event;
                wd->event_callback(close_event);
            }
        });

        glfwSetKeyCallback(this->m_window,
            [](GLFWwindow* window, int32_t key, int32_t scancode, int32_t action, int32_t mods) {
            window_data* wd = (window_data*)glfwGetWindowUserPointer(window);

            if (wd->event_callback != nullptr) {
                switch (action) {
                case GLFW_PRESS:
                    // todo: key pressed event
                    break;
                case GLFW_RELEASE:
                    // todo: key released event
                    break;
                case GLFW_REPEAT:
                    // todo: key pressed event but with a flag i guess
                    break;
                }
            }
        });

        glfwSetCharCallback(this->m_window, [](GLFWwindow* window, uint32_t scancode) {
            window_data* wd = (window_data*)glfwGetWindowUserPointer(window);

            if (wd->event_callback != nullptr) {
                // todo: event stuff
            }
        });
    }
}