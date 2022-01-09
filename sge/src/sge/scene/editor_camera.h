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
#include "sge/events/input_events.h"
namespace sge {
    class editor_camera {
    public:
        editor_camera() { recalculate_projection(); }

        editor_camera(const editor_camera&) = delete;
        editor_camera& operator=(const editor_camera&) = delete;

        glm::mat4 get_view_projection_matrix() const;
        void update_viewport_size(uint32_t width, uint32_t height);

        void on_update(timestep ts);
        void on_event(event& e);

        void enable_input() { m_input_enabled = true; }
        void disable_input() { m_input_enabled = false; }

        glm::vec2 get_position() const { return m_position; }
        float get_view_size() const { return m_view_size; }
        float get_aspect_ratio() const { return m_aspect_ratio; }

    private:
        bool on_scroll(mouse_scrolled_event& e);

        void recalculate_projection();
        glm::mat4 m_projection;

        glm::vec2 m_position = glm::vec2(0.f);

        float m_aspect_ratio = 0.f;
        uint32_t m_viewport_width = 0;
        uint32_t m_viewport_height = 0;

        float m_view_size = 10.f;

        bool m_input_enabled = false;
        std::optional<glm::vec2> m_last_mouse_position;
    };
} // namespace sge