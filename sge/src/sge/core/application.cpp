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

        // todo: window
    }

    void application::init() {
        spdlog::info("initializing application: {0}...", this->m_title);

        // todo: layers

        this->load_content();
    }

    void application::shutdown() {
        spdlog::info("shutting down application: {0}...", this->m_title);

        this->unload_content();
    }
}