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
#include "sge/core/application.h"
#include "sge/renderer/renderer.h"
namespace sge {
    static ref<application> app_instance;
    void application::set(ref<application> app) { app_instance = app; }
    ref<application> application::get() { return app_instance; }

    application::application(const std::string& title) {
        this->m_title = title;
        this->m_running = false;
        this->m_minimized = false;
    }

    void application::on_event(event& e) {
        event_dispatcher dispatcher(e);

        dispatcher.dispatch<window_close_event>(SGE_BIND_EVENT_FUNC(application::on_window_close));
        dispatcher.dispatch<window_resize_event>(
            SGE_BIND_EVENT_FUNC(application::on_window_resize));

        for (auto& _layer : this->m_layer_stack) {
            if (e.handled) {
                break;
            }
            _layer->on_event(e);
        }
    }

    void application::init() {
        spdlog::info("initializing application: {0}...", this->m_title);

        this->m_window = window::create(this->m_title, 1600, 900);
        this->m_window->set_event_callback(SGE_BIND_EVENT_FUNC(application::on_event));

        renderer::init();
        this->m_swapchain = swapchain::create(this->m_window);

        this->init_app();
    }

    void application::shutdown() {
        spdlog::info("shutting down application: {0}...", this->m_title);

        this->shutdown_app();

        this->m_swapchain.reset();
        renderer::shutdown();

        this->m_window.reset();
    }

    void application::run() {
        if (this->m_running) {
            throw std::runtime_error("cannot recursively call run()");
        }
        this->m_running = true;

        while (this->m_running) {
            this->m_swapchain->new_frame();
            renderer::new_frame();

            size_t current_image = this->m_swapchain->get_current_image_index();
            auto& cmdlist = this->m_swapchain->get_command_list(current_image);
            cmdlist.begin();
            this->m_swapchain->begin(cmdlist, glm::vec4(0.3f, 0.3f, 0.3f, 1.f));
            renderer::set_command_list(cmdlist);

            // todo: timestep

            if (!this->m_minimized) {
                for (auto it = this->m_layer_stack.rbegin(); it != this->m_layer_stack.rend();
                     it++) {
                    (*it)->on_update();
                }
            }
            
            this->m_swapchain->end(cmdlist);
            cmdlist.end();

            this->m_swapchain->present();
            this->m_window->on_update();
        }
    }

    bool application::on_window_close(window_close_event& e) {
        this->m_running = false;
        return true;
    }

    bool application::on_window_resize(window_resize_event& e) {
        uint32_t width = e.get_width();
        uint32_t height = e.get_height();

        this->m_minimized = (width == 0 || height == 0);
        if (!this->m_minimized) {
            this->m_swapchain->on_resize(width, height);
        }

        return false;
    }
} // namespace sge