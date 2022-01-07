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
namespace sge {
    enum class projection_type { orthographic = 0, perspective };

    class runtime_camera {
    public:
        runtime_camera() { this->recalculate_projection(); }

        runtime_camera(const runtime_camera&) = default;
        runtime_camera& operator=(const runtime_camera&) = default;

        void set_perspective(float vertical_fov, float near_plane, float far_plane);
        void set_orthographic(float size, float near_plane, float far_plane);

        void set_render_target_size(uint32_t width, uint32_t height);

        float get_vertical_fov() const { return this->m_fov; }
        void set_vertical_fov(float fov) {
            this->m_fov = fov;
            this->recalculate_projection();
        }

        float get_perspective_near_plane() const { return this->m_perspective_clips.near; }
        void set_perspective_near_plane(float near_plane) {
            this->m_perspective_clips.near = near_plane;
            this->recalculate_projection();
        }

        float get_perspective_far_plane() const { return this->m_perspective_clips.far; }
        void set_perspective_far_plane(float far_plane) {
            this->m_perspective_clips.far = far_plane;
            this->recalculate_projection();
        }

        float get_orthographic_size() const { return this->m_orthographic_size; }
        void set_orthographic_size(float size) {
            this->m_orthographic_size = size;
            this->recalculate_projection();
        }

        float get_orthographic_near_plane() const { return this->m_orthographic_clips.near; }
        void set_orthographic_near_plane(float near_plane) {
            this->m_orthographic_clips.near = near_plane;
            this->recalculate_projection();
        }

        float get_orthographic_far_plane() const { return this->m_orthographic_clips.far; }
        void set_orthographic_far_plane(float far_plane) {
            this->m_orthographic_clips.far = far_plane;
            this->recalculate_projection();
        }

        const glm::mat4& get_projection() const { return this->m_projection; }
        projection_type get_projection_type() const { return this->m_type; }
        void set_projection_type(projection_type type) {
            this->m_type = type;
            this->recalculate_projection();
        }

    private:
        struct clips {
            float near, far;
        };

        void recalculate_projection();
        glm::mat4 m_projection;

        projection_type m_type = projection_type::orthographic;

        float m_fov = glm::radians(45.f);
        clips m_perspective_clips = { 0.01f, 1000.f };

        float m_orthographic_size = 10.f;
        clips m_orthographic_clips = { -1.f, 1.f };

        float m_aspect_ratio;
    };
} // namespace sge