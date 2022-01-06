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
#include "sge/core/application.h"
#include "sge/core/window.h"
#include "sge/renderer/renderer.h"
#include "sge/scene/entity.h"
#include "sge/scene/components.h"

namespace sge {
    entity scene::create_entity(const std::string& name) {
        entity e = { m_registry.create(), this };

        e.add_component<transform_component>();
        auto& t = e.add_component<tag_component>();
        t.tag = name.empty() ? "Entity" : name;

        return e;
    }

    void scene::destroy_entity(entity e) {
        if (e.has_all<native_script_component>()) {
            auto& nsc = e.get_component<native_script_component>();
            if (nsc.script != nullptr) {
                nsc.destroy(&nsc);
            }
        }

        m_registry.destroy(e);
    }

    void scene::on_runtime_update(timestep ts) {
        {
            auto view = m_registry.view<native_script_component>();
            for (auto entt_entity : view) {
                entity entity(entt_entity, this);
                auto& nsc = entity.get_component<native_script_component>();

                if (nsc.script == nullptr && nsc.instantiate != nullptr) {
                    nsc.instantiate(&nsc, entity);
                }

                if (nsc.script != nullptr) {
                    nsc.script->on_update(ts);
                }
            }
        }

        runtime_camera* main_camera = nullptr;
        glm::mat4 camera_transform;
        {
            auto view = m_registry.view<transform_component, camera_component>();
            for (entt::entity id : view) {
                const auto& [camera_data, transform] =
                    m_registry.get<camera_component, transform_component>(id);
                
                if (camera_data.primary) {
                    main_camera = &camera_data.camera;
                    camera_transform = transform.get_transform();
                    break;
                }
            }
        }

        if (main_camera != nullptr) {
            glm::mat4 projection = main_camera->get_projection();
            render(projection * glm::inverse(camera_transform));
        }
    }

    void scene::on_editor_update(timestep ts) {
        // todo(nora): get editor camera data
        glm::mat4 projection;
        {
            static constexpr float camera_size = 10.f;
            float aspect_ratio = (float)m_viewport_width / (float)m_viewport_height;
            
            float left = -camera_size * aspect_ratio / 2.f;
            float right = camera_size * aspect_ratio / 2.f;
            float bottom = -camera_size / 2.f;
            float top = camera_size / 2.f;

            projection = glm::ortho(left, right, bottom, top, -1.f, 1.f);
        }

        render(projection);
    }

    void scene::on_event(event& e) {
        event_dispatcher dispatcher(e);

        auto view = m_registry.view<native_script_component>();
        for (auto entt_entity : view) {
            entity entity(entt_entity, this);
            auto& nsc = entity.get_component<native_script_component>();
            
            if (nsc.script == nullptr && nsc.instantiate != nullptr) {
                nsc.instantiate(&nsc, entity);
            }

            if (nsc.script != nullptr) {
                nsc.script->on_event(e);
            }
        }
    }

    void scene::set_viewport_size(uint32_t width, uint32_t height) {
        m_viewport_width = width;
        m_viewport_height = height;

        auto view = m_registry.view<camera_component>();
        for (entt::entity id : view) {
            auto& camera = view.get<camera_component>(id);
            camera.camera.set_render_target_size(width, height);
        }
    }

    void scene::render(const glm::mat4& view_projection) {
        renderer::begin_scene(view_projection);

        auto group = m_registry.group<transform_component>(entt::get<sprite_renderer_component>);
        for (auto entity : group) {
            auto [transform, sprite] =
                group.get<transform_component, sprite_renderer_component>(entity);

            glm::vec2 position = transform.translation - transform.scale / 2.f;
            float rotation = transform.rotation;
            glm::vec2 size = transform.scale;
            if (sprite.texture) {
                renderer::draw_rotated_quad(position, rotation, size, sprite.color, sprite.texture);
            } else {
                renderer::draw_rotated_quad(position, rotation, size, sprite.color);
            }
        }

        renderer::end_scene();
    }
} // namespace sge