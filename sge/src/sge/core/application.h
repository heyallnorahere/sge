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
#include "sge/events/event.h"
#include "sge/events/window_events.h"
namespace sge {
    class application : public ref_counted {
    public:
        static void set(ref<application> app);
        static ref<application> get();

        application(const std::string& title);

        void init();
        void shutdown();

        void on_event(event& e);

        ref<window> get_window() { return this->m_window; }

    private:
        bool on_window_resize(window_resize_event& e);
        bool on_window_close(window_close_event& e);

    protected:
        virtual void init_app() { }
        virtual void shutdown_app() { }

        std::string m_title;
        ref<window> m_window;
    };
}