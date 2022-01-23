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
#include "sge/core/input.h"
#include "sge/imgui/imgui_layer.h"
#include "sge/script/script_engine.h"
#include "sge/asset/asset_serializers.h"
#include "sge/asset/project.h"

extern sge::application* create_app_instance();
namespace sge {
    static std::unique_ptr<application> app_instance;

    void application::create() {
        app_instance = std::unique_ptr<application>(::create_app_instance());
    }

    void application::destroy() {
        app_instance.reset();
    }

    application& application::get() { return *app_instance; }

    application::application(const std::string& title) {
        m_title = title;
        m_running = false;
        m_minimized = false;
    }

    void application::on_event(event& e) {
        event_dispatcher dispatcher(e);

        dispatcher.dispatch<window_close_event>(SGE_BIND_EVENT_FUNC(application::on_window_close));
        dispatcher.dispatch<window_resize_event>(
            SGE_BIND_EVENT_FUNC(application::on_window_resize));

        input::on_event(e);

        for (auto& _layer : m_layer_stack) {
            if (e.handled) {
                break;
            }
            _layer->on_event(e);
        }
    }

    void application::init() {
        spdlog::info("initializing application: {0}...", m_title);

        input::init();
        m_window = window::create(get_window_title(), 1600, 900);
        m_window->set_event_callback(SGE_BIND_EVENT_FUNC(application::on_event));

        renderer::init();
        m_swapchain = swapchain::create(m_window);

        asset_serializer::init();
        script_engine::init();

        m_imgui_layer = new imgui_layer;
        push_overlay(m_imgui_layer);

        project::init();
        on_init();
    }

    void application::shutdown() {
        spdlog::info("shutting down application: {0}...", m_title);

        renderer::clear_render_data();

        on_shutdown();
        project::shutdown();

        pop_overlay(m_imgui_layer);
        delete m_imgui_layer;
        m_imgui_layer = nullptr;

        script_engine::shutdown();

        m_swapchain.reset();
        renderer::shutdown();

        m_window.reset();
        input::shutdown();
    }

    void application::run() {
        if (m_running) {
            throw std::runtime_error("cannot recursively call run()");
        }
        m_running = true;

        while (m_running) {
            if (!m_minimized) {
                m_swapchain->new_frame();
                renderer::new_frame();

                size_t current_image = m_swapchain->get_current_image_index();
                auto& cmdlist = m_swapchain->get_command_list(current_image);
                cmdlist.begin();
                renderer::set_command_list(cmdlist);

                auto pass = m_swapchain->get_render_pass();
                renderer::push_render_pass(pass, glm::vec4(0.3f, 0.3f, 0.3f, 1.f));

                timestep ts;
                {
                    using namespace std::chrono;

                    static high_resolution_clock clock;
                    static high_resolution_clock::time_point t0 = clock.now();
                    high_resolution_clock::time_point t1 = clock.now();

                    ts = duration_cast<timestep>(t1 - t0);
                    t0 = t1;
                }

                for (auto it = m_layer_stack.rbegin(); it != m_layer_stack.rend();
                     it++) {
                    (*it)->on_update(ts);
                }

                renderer::begin_render_pass();
                m_imgui_layer->begin();
                for (auto& layer : m_layer_stack) {
                    layer->on_imgui_render();
                }
                m_imgui_layer->end(cmdlist);

                if (renderer::pop_render_pass() != pass) {
                    throw std::runtime_error("a render pass was pushed, but not popped!");
                }
                cmdlist.end();

                m_swapchain->present();
            }

            m_window->on_update();
        }
    }

    bool application::on_window_close(window_close_event& e) {
        m_running = false;
        return true;
    }

    bool application::on_window_resize(window_resize_event& e) {
        uint32_t width = e.get_width();
        uint32_t height = e.get_height();

        m_minimized = (width == 0 || height == 0);
        if (!m_minimized) {
            m_swapchain->on_resize(width, height);
        }

        return false;
    }
} // namespace sge