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

    void scene::destroy_entity(entity e) { m_registry.destroy(e); }

    void scene::on_update(timestep ts) {
        // todo(nora): update script components

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
            renderer::begin_scene(projection * glm::inverse(camera_transform));

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
    }

    void scene::on_event(event& e) {
        event_dispatcher dispatcher(e);

        dispatcher.dispatch<window_resize_event>(SGE_BIND_EVENT_FUNC(scene::on_resize));

        // todo(nora): pass to script components
    }

    bool scene::on_resize(window_resize_event& e) {
        auto view = m_registry.view<camera_component>();
        for (entt::entity id : view) {
            auto& camera = view.get<camera_component>(id);
            camera.camera.set_render_target_size(e.get_width(), e.get_height());
        }

        return false;
    }
} // namespace sge