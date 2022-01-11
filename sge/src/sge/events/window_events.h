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
namespace sge {
    class window_close_event : public event {
    public:
        window_close_event() = default;

        EVENT_ID_DECL(window_close)
    };
    class window_resize_event : public event {
    public:
        window_resize_event(uint32_t width, uint32_t height) {
            m_width = width;
            m_height = height;
        }

        uint32_t get_width() { return m_width; }
        uint32_t get_height() { return m_height; }

        EVENT_ID_DECL(window_resize)

    private:
        uint32_t m_width, m_height;
    };
} // namespace sge