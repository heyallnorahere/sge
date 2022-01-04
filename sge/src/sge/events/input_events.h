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
} // namespace sge