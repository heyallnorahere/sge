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
#include "sge/scene/editor_camera.h"
#include "sge/core/input.h"
namespace sge {
    glm::mat4 editor_camera::get_view_projection_matrix() const {
        glm::mat4 transformation(1.f);
        transformation = glm::translate(transformation, glm::vec3(m_position, 0.f));
        return m_projection * glm::inverse(transformation);
    }

    void editor_camera::update_viewport_size(uint32_t width, uint32_t height) {
        m_aspect_ratio = (float)width / (float)height;
        m_viewport_width = width;
        m_viewport_height = height;

        recalculate_projection();
    }

    void editor_camera::on_update(timestep ts) {
        if (input::get_mouse_button(mouse_button::right) && m_input_enabled) {
            glm::vec2 mouse_position = input::get_mouse_position();
            if (!m_last_mouse_position.has_value()) {
                m_last_mouse_position = mouse_position;
            }

            glm::vec2 offset =
                (mouse_position - m_last_mouse_position.value()) * glm::vec2(1.f, -1.f);
            m_last_mouse_position = mouse_position;

            glm::vec2 viewport_size = glm::vec2(m_viewport_width, m_viewport_height);
            glm::vec2 view_size = glm::vec2(m_view_size * m_aspect_ratio, m_view_size);
            m_position -= offset * view_size / viewport_size;
        } else {
            m_last_mouse_position.reset();
        }
    }

    void editor_camera::on_event(event& e) {
        event_dispatcher dispatcher(e);

        dispatcher.dispatch<mouse_scrolled_event>(SGE_BIND_EVENT_FUNC(editor_camera::on_scroll));
    }

    bool editor_camera::on_scroll(mouse_scrolled_event& e) {
        if (!m_input_enabled) {
            return false;
        }

        m_view_size *= glm::pow(2.f, -e.get_offset().y);
        recalculate_projection();

        return true;
    }

    void editor_camera::recalculate_projection() {
        float left = -m_view_size * m_aspect_ratio / 2.f;
        float right = m_view_size * m_aspect_ratio / 2.f;
        float bottom = -m_view_size / 2.f;
        float top = m_view_size / 2.f;

        m_projection = glm::ortho(left, right, bottom, top, -1.f, 1.f);
    }
} // namespace sge