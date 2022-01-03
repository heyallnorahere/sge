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
#include <sge/renderer/renderer.h>
#include "entity.h"
#include "components.h"

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
        // Temporary: construct static camera with aspect ratio from app window
        auto window = sge::application::get().get_window();
        float aspect_ratio = (float)window->get_width() / (float)window->get_height();

        glm::mat4 projection;
        {
            static constexpr float orthographic_size = 10.f;

            float left = -orthographic_size * aspect_ratio / 2.f;
            float right = orthographic_size * aspect_ratio / 2.f;
            float bottom = -orthographic_size / 2.f;
            float top = orthographic_size / 2.f;

            projection = glm::ortho(left, right, bottom, top, -1.f, 1.f);
        }

        glm::vec2 translation = glm::vec2(0.f, 0.f);
        glm::mat4 model = glm::translate(glm::mat4(1.f), glm::vec3(translation, 0.f));

        sge::renderer::begin_scene(projection * glm::inverse(model));

        auto group = m_registry.group<transform_component>(entt::get<sprite_renderer_component>);
        for (auto entity : group) {
            auto [transform, sprite] =
                group.get<transform_component, sprite_renderer_component>(entity);

            glm::vec2 position = transform.translation - transform.scale / 2.f;
            float rotation = transform.rotation;
            glm::vec2 size = transform.scale;
            if (sprite.texture) {
                sge::renderer::draw_rotated_quad(position, rotation, size, sprite.color,
                                                 sprite.texture);
            } else {
                sge::renderer::draw_rotated_quad(position, rotation, size, sprite.color);
            }
        }
        sge::renderer::end_scene();
    }

} // namespace sge