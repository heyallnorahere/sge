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
#include "sge/scene/runtime_camera.h"
namespace sge {
    void runtime_camera::set_perspective(float vertical_fov, float near_plane, float far_plane) {
        m_fov = vertical_fov;
        m_perspective_clips = { near_plane, far_plane };
        recalculate_projection();
    }

    void runtime_camera::set_orthographic(float size, float near_plane, float far_plane) {
        m_orthographic_size = size;
        m_orthographic_clips = { near_plane, far_plane };
        recalculate_projection();
    }

    void runtime_camera::set_render_target_size(uint32_t width, uint32_t height) {
        m_aspect_ratio = (float)width / (float)height;
        recalculate_projection();
    }

    void runtime_camera::recalculate_projection() {
        switch (m_type) {
        case projection_type::orthographic: {
            float ortho_left = -m_orthographic_size * m_aspect_ratio / 2.f;
            float ortho_right = m_orthographic_size * m_aspect_ratio / 2.f;
            float ortho_bottom = -m_orthographic_size / 2.f;
            float ortho_top = m_orthographic_size / 2.f;

            m_projection =
                glm::ortho(ortho_left, ortho_right, ortho_bottom, ortho_top,
                           m_orthographic_clips.near, m_orthographic_clips.far);
        } break;
        case projection_type::perspective:
            m_projection =
                glm::perspective(glm::radians(m_fov), m_aspect_ratio,
                                 m_perspective_clips.near, m_orthographic_clips.far);
            break;
        default:
            throw std::runtime_error("invalid projection type!");
        }
    }
} // namespace sge
