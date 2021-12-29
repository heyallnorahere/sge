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
#include "sge/core/application.h"
namespace sge {
    static ref<application> app_instance;
    void application::set(ref<application> app) {
        app_instance = app;
    }
    ref<application> application::get() {
        return app_instance;
    }

    application::application(const std::string& title) {
        this->m_title = title;
    }

    void application::init() {
        spdlog::info("initializing application: {0}...", this->m_title);

        this->m_window = window::create(this->m_title, 1600, 900);
        this->m_window->set_event_callback(SGE_BIND_EVENT_FUNC(application::on_event));

        // todo: layers

        this->init_app();
    }

    void application::shutdown() {
        spdlog::info("shutting down application: {0}...", this->m_title);

        this->shutdown_app();
    }

    void application::on_event(event& e) {
        event_dispatcher dispatcher(e);

        dispatcher.dispatch<window_close_event>(SGE_BIND_EVENT_FUNC(application::on_window_close));
        dispatcher.dispatch<window_resize_event>(SGE_BIND_EVENT_FUNC(application::on_window_resize));

        // todo: layers
    }

    bool application::on_window_close(window_close_event& e) {
        spdlog::info("window closed");
        return true;
    }

    bool application::on_window_resize(window_resize_event& e) {
        spdlog::info("window resized to size: ({0}, {1})", e.get_width(), e.get_height());
        return true;
    }
}