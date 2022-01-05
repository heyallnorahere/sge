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
#include "sge/core/key_codes.h"
namespace sge {
    class key_pressed_event : public event {
    public:
        key_pressed_event(key_code key, uint32_t repeat_count) {
            this->m_key = key;
            this->m_repeat_count = repeat_count;
        }

        key_code get_key() { return this->m_key; }
        uint32_t get_repeat_count() { return this->m_repeat_count; }

        EVENT_ID_DECL(key_pressed)

    private:
        key_code m_key;
        uint32_t m_repeat_count;
    };

    class key_released_event : public event {
    public:
        key_released_event(key_code key) { this->m_key = key; }

        key_code get_key() { return this->m_key; }

        EVENT_ID_DECL(key_released)

    private:
        key_code m_key;
    };

    class key_typed_event : public event {
    public:
        key_typed_event(key_code key) { this->m_key = key; }

        key_code get_key() { return this->m_key; }

        EVENT_ID_DECL(key_typed)

    private:
        key_code m_key;
    };

    class mouse_moved_event : public event {
    public:
        mouse_moved_event(glm::vec2 position) { this->m_position = position; }

        glm::vec2 get_position() { return this->m_position; }

        EVENT_ID_DECL(mouse_moved)

    private:
        glm::vec2 m_position;
    };

    class mouse_scrolled_event : public event {
    public:
        mouse_scrolled_event(glm::vec2 offset) { this->m_offset = offset; }

        glm::vec2 get_offset() { return this->m_offset; }

        EVENT_ID_DECL(mouse_scrolled)

    private:
        glm::vec2 m_offset;
    };

    class mouse_button_event : public event {
    public:
        mouse_button_event(mouse_button button, bool released) {
            this->m_button = button;
            this->m_released = released;
        }

        mouse_button get_button() { return this->m_button; }
        bool get_released() { return this->m_released; }

        EVENT_ID_DECL(mouse_button);

    private:
        mouse_button m_button;
        bool m_released;
    };
} // namespace sge