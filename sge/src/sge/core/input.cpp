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
#include "sge/core/input.h"
#include "sge/core/application.h"
#include "sge/events/input_events.h"
namespace sge {
    struct input_data_t {
        std::map<key_code, bool> key_status;
        std::map<mouse_button, bool> mouse_button_status;
    };
    static std::unique_ptr<input_data_t> input_data;

    void input::init() { input_data = std::make_unique<input_data_t>(); }
    void input::shutdown() { input_data.reset(); }

    static bool input_on_pressed(key_pressed_event& e) {
        if (e.get_repeat_count() == 0) {
            key_code code = e.get_key();
            input_data->key_status[code] = true;
        }

        return false;
    }

    static bool input_on_released(key_released_event& e) {
        key_code code = e.get_key();
        input_data->key_status[code] = false;

        return false;
    }

    static bool input_mouse_button(mouse_button_event& e) {
        mouse_button button = e.get_button();
        input_data->mouse_button_status[button] = !e.get_released();

        return false;
    }

    void input::on_event(event& e) {
        event_dispatcher dispatcher(e);

        dispatcher.dispatch<key_pressed_event>(input_on_pressed);
        dispatcher.dispatch<key_released_event>(input_on_released);
        dispatcher.dispatch<mouse_button_event>(input_mouse_button);
    }

    bool input::get_key(key_code code) {
        if (input_data->key_status.find(code) == input_data->key_status.end()) {
            return false;
        }

        return input_data->key_status[code];
    }

    bool input::get_mouse_button(mouse_button button) {
        if (input_data->mouse_button_status.find(button) == input_data->mouse_button_status.end()) {
            return false;
        }

        return input_data->mouse_button_status[button];
    }
} // namespace sge