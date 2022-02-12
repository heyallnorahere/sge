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
#ifdef SGE_USE_VULKAN
#include "sge/platform/vulkan/vulkan_base.h"
#endif
#include "sge/platform/desktop/desktop_window.h"
#include "sge/events/window_events.h"
#include "sge/events/input_events.h"
#include "sge/core/input.h"
#include "sge/renderer/renderer.h"
#ifdef SGE_USE_DIRECTX
#include "sge/platform/directx/directx_base.h"
#include "sge/platform/directx/directx_command_queue.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>
namespace sge {
    static uint32_t glfw_window_count = 0;

    desktop_window::desktop_window(const std::string& title, uint32_t width, uint32_t height) {
        m_data.title = title;
        m_data.width = width;
        m_data.height = height;
        m_data.event_callback = nullptr;
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

        m_window =
            glfwCreateWindow(m_data.width, m_data.height, m_data.title.c_str(), nullptr, nullptr);
        if (m_window == nullptr) {
            throw std::runtime_error("could not create glfw window!");
        }

        setup_event_callbacks();
    }

    desktop_window::~desktop_window() {
        glfwDestroyWindow(m_window);
        glfw_window_count--;

        if (glfw_window_count == 0) {
            glfwTerminate();
        }
    }

    void desktop_window::on_update() { glfwPollEvents(); }

    void desktop_window::set_title(const std::string& title) {
        glfwSetWindowTitle(m_window, title.c_str());
    }

    void desktop_window::set_event_callback(event_callback_t callback) {
        m_data.event_callback = callback;
    }

    void* desktop_window::create_render_surface(void* params) {
#ifdef SGE_USE_DIRECTX
        // for directx, let's just create the swapchain
        {
            HWND native_window = glfwGetWin32Window(m_window);

            ComPtr<IDXGIFactory4> factory;
            create_factory(factory);

            ComPtr<IDXGISwapChain4> swapchain4;
            {
                auto queue = renderer::get_queue(command_list_type::graphics);
                auto dx_queue = queue.as<directx_command_queue>();
                auto cmdqueue = dx_queue->get_queue();

                ComPtr<IDXGISwapChain1> swapchain1;
                auto desc = (DXGI_SWAP_CHAIN_DESC1*)params;
                COM_assert(factory->CreateSwapChainForHwnd(cmdqueue.Get(), native_window, desc,
                                                           nullptr, nullptr, &swapchain1));
                COM_assert(swapchain1.As(&swapchain4));
            }

            COM_assert(factory->MakeWindowAssociation(native_window, DXGI_MWA_NO_ALT_ENTER));

            auto ptr = swapchain4.Get();
            ptr->AddRef();
            return ptr;
        }
#endif

#ifdef SGE_USE_VULKAN
        {
            auto instance = (VkInstance)params;

            VkSurfaceKHR surface;
            VkResult result = glfwCreateWindowSurface(instance, m_window, nullptr, &surface);
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
#else
        throw std::runtime_error("vulkan is not enabled!");
#endif
    }

    void desktop_window::setup_event_callbacks() {
        glfwSetWindowUserPointer(m_window, &m_data);

        glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int32_t width, int32_t height) {
            window_data* wd = (window_data*)glfwGetWindowUserPointer(window);
            wd->width = (uint32_t)width;
            wd->height = (uint32_t)height;

            if (wd->event_callback != nullptr) {
                window_resize_event resize_event(wd->width, wd->height);
                wd->event_callback(resize_event);
            }
        });

        glfwSetWindowCloseCallback(m_window, [](GLFWwindow* window) {
            window_data* wd = (window_data*)glfwGetWindowUserPointer(window);

            if (wd->event_callback != nullptr) {
                window_close_event close_event;
                wd->event_callback(close_event);
            }
        });

        glfwSetMouseButtonCallback(
            m_window, [](GLFWwindow* window, int32_t button, int32_t action, int32_t mods) {
                window_data* wd = (window_data*)glfwGetWindowUserPointer(window);

                if (wd->event_callback != nullptr) {
                    mouse_button mbutton;
                    switch (button) {
                    case GLFW_MOUSE_BUTTON_LEFT:
                        mbutton = mouse_button::left;
                        break;
                    case GLFW_MOUSE_BUTTON_RIGHT:
                        mbutton = mouse_button::right;
                        break;
                    case GLFW_MOUSE_BUTTON_MIDDLE:
                        mbutton = mouse_button::middle;
                        break;
                    default:
                        // unhandled
                        return;
                    }

                    mouse_button_event e(mbutton, action == GLFW_RELEASE);
                    wd->event_callback(e);
                }
            });

        glfwSetKeyCallback(m_window, [](GLFWwindow* window, int32_t key, int32_t scancode,
                                        int32_t action, int32_t mods) {
            window_data* wd = (window_data*)glfwGetWindowUserPointer(window);

            if (wd->event_callback != nullptr) {
                key_code code;
                switch (key) {
#define MAP_KEY(key)                                                                               \
    case GLFW_KEY_##key:                                                                           \
        code = key_code::key;                                                                      \
        break
                    MAP_KEY(SPACE);
                    MAP_KEY(APOSTROPHE);
                    MAP_KEY(COMMA);
                    MAP_KEY(MINUS);
                    MAP_KEY(PERIOD);
                    MAP_KEY(SLASH);
                case GLFW_KEY_0:
                    code = key_code::ZERO;
                    break;
                case GLFW_KEY_1:
                    code = key_code::ONE;
                    break;
                case GLFW_KEY_2:
                    code = key_code::TWO;
                    break;
                case GLFW_KEY_3:
                    code = key_code::THREE;
                    break;
                case GLFW_KEY_4:
                    code = key_code::FOUR;
                    break;
                case GLFW_KEY_5:
                    code = key_code::FIVE;
                    break;
                case GLFW_KEY_6:
                    code = key_code::SIX;
                    break;
                case GLFW_KEY_7:
                    code = key_code::SEVEN;
                    break;
                case GLFW_KEY_8:
                    code = key_code::EIGHT;
                    break;
                case GLFW_KEY_9:
                    code = key_code::NINE;
                    break;
                    MAP_KEY(SEMICOLON);
                    MAP_KEY(EQUAL);
                    MAP_KEY(A);
                    MAP_KEY(B);
                    MAP_KEY(C);
                    MAP_KEY(D);
                    MAP_KEY(E);
                    MAP_KEY(F);
                    MAP_KEY(G);
                    MAP_KEY(H);
                    MAP_KEY(I);
                    MAP_KEY(J);
                    MAP_KEY(K);
                    MAP_KEY(L);
                    MAP_KEY(M);
                    MAP_KEY(N);
                    MAP_KEY(O);
                    MAP_KEY(P);
                    MAP_KEY(Q);
                    MAP_KEY(R);
                    MAP_KEY(S);
                    MAP_KEY(T);
                    MAP_KEY(U);
                    MAP_KEY(V);
                    MAP_KEY(W);
                    MAP_KEY(X);
                    MAP_KEY(Y);
                    MAP_KEY(Z);
                    MAP_KEY(LEFT_BRACKET);
                    MAP_KEY(BACKSLASH);
                    MAP_KEY(RIGHT_BRACKET);
                    MAP_KEY(GRAVE_ACCENT);
                    MAP_KEY(ESCAPE);
                    MAP_KEY(ENTER);
                    MAP_KEY(TAB);
                    MAP_KEY(BACKSPACE);
                    MAP_KEY(INSERT);
                    MAP_KEY(DELETE);
                    MAP_KEY(RIGHT);
                    MAP_KEY(LEFT);
                    MAP_KEY(DOWN);
                    MAP_KEY(UP);
                    MAP_KEY(F1);
                    MAP_KEY(F2);
                    MAP_KEY(F3);
                    MAP_KEY(F4);
                    MAP_KEY(F5);
                    MAP_KEY(F6);
                    MAP_KEY(F7);
                    MAP_KEY(F8);
                    MAP_KEY(F9);
                    MAP_KEY(F10);
                    MAP_KEY(F11);
                    MAP_KEY(F12);
                    MAP_KEY(F13);
                    MAP_KEY(F14);
                    MAP_KEY(F15);
                    MAP_KEY(F16);
                    MAP_KEY(F17);
                    MAP_KEY(F18);
                    MAP_KEY(F19);
                    MAP_KEY(F20);
                    MAP_KEY(F21);
                    MAP_KEY(F22);
                    MAP_KEY(F23);
                    MAP_KEY(F24);
                    MAP_KEY(F25);
                    MAP_KEY(LEFT_SHIFT);
                    MAP_KEY(LEFT_CONTROL);
                    MAP_KEY(LEFT_ALT);
                    MAP_KEY(RIGHT_SHIFT);
                    MAP_KEY(RIGHT_CONTROL);
                    MAP_KEY(RIGHT_ALT);
#undef MAP_KEY
                default:
                    // not handled
                    return;
                }

                event* e = nullptr;
                switch (action) {
                case GLFW_PRESS:
                    e = new key_pressed_event(code, 0);
                    break;
                case GLFW_RELEASE:
                    e = new key_released_event(code);
                    break;
                case GLFW_REPEAT:
                    e = new key_pressed_event(code, 1);
                    break;
                }

                wd->event_callback(*e);
                delete e;
            }
        });

        glfwSetScrollCallback(m_window, [](GLFWwindow* window, double x, double y) {
            window_data* wd = (window_data*)glfwGetWindowUserPointer(window);

            if (wd->event_callback != nullptr) {
                mouse_scrolled_event e(glm::vec2((float)x, (float)y));
                wd->event_callback(e);
            }
        });

        glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double x, double y) {
            auto position = glm::vec2((float)x, (float)y);
            if (glm::length(input::get_mouse_position()) < 0.0001f) {
                input::set_mouse_position(position);
            }

            window_data* wd = (window_data*)glfwGetWindowUserPointer(window);

            if (wd->event_callback != nullptr) {
                mouse_moved_event e(position);
                wd->event_callback(e);
            }
        });
    }
} // namespace sge
