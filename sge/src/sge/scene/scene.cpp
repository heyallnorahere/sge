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

#include <box2d/b2_world.h>
#include <box2d/b2_body.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_circle_shape.h>

namespace sge {

    static b2BodyType rigid_body_type_to_box2d_body(rigid_body_component::body_type bt) {
        switch (bt) {
        case rigid_body_component::body_type::static_:
            return b2_staticBody;
        case rigid_body_component::body_type::kinematic:
            return b2_kinematicBody;
        case rigid_body_component::body_type::dynamic:
            return b2_dynamicBody;
        }

        throw std::runtime_error("invalid body type");
        return b2_staticBody;
    }

    scene::~scene() {
        if (m_physics_world != nullptr) {
            delete m_physics_world;
        }
    }

    entity scene::create_entity(const std::string& name) {
        entity e(this->m_registry.create(), this);

        e.add_component<id_component>();
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

    void scene::clear() {
        m_registry.each([this](entt::entity id) {
            entity e(id, this);
            if (e.has_all<native_script_component>()) {
                auto& nsc = e.get_component<native_script_component>();
                if (nsc.script != nullptr) {
                    nsc.destroy(&nsc);
                }
            }
        });

        m_registry.clear();
    }

    void scene::on_start() {
        // Initialize the box2d physics engine
        m_physics_world = new b2World({ 0.f, -9.8f });
        auto view = m_registry.view<rigid_body_component>();
        for (auto id : view) {
            entity e(id, this);
            auto& transform = e.get_component<transform_component>();
            auto& rb = e.get_component<rigid_body_component>();

            b2BodyDef body_def;
            body_def.type = rigid_body_type_to_box2d_body(rb.type);
            body_def.position.Set(transform.translation.x, transform.translation.y);
            body_def.angle = glm::radians(transform.rotation);
            b2Body* body = m_physics_world->CreateBody(&body_def);
            body->SetFixedRotation(rb.fixed_rotation);
            rb.runtime_body = body;

            if (e.has_all<box_collider_component>()) {
                auto& bc = e.get_component<box_collider_component>();

                b2PolygonShape box_shape;
                box_shape.SetAsBox(bc.size.x * transform.scale.x, bc.size.y * transform.scale.y);

                b2FixtureDef fixture_def;
                fixture_def.shape = &box_shape;
                fixture_def.density = bc.density;
                fixture_def.friction = bc.friction;
                fixture_def.restitution = bc.restitution;
                fixture_def.restitutionThreshold = bc.restitution_threadhold;
                auto fixture = body->CreateFixture(&fixture_def);
                bc.runtime_fixture = fixture;
            }
        }
    }

    void scene::on_stop() {
        delete m_physics_world;
        m_physics_world = nullptr;
    }

    void scene::on_runtime_update(timestep ts) {
        // Scripts
        {
            auto view = this->m_registry.view<native_script_component>();
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

        // Physics
        {
            static constexpr int32_t velocity_iterations = 6;
            static constexpr int32_t position_iterations = 2;

            auto view = m_registry.view<transform_component, rigid_body_component>();
            for (entt::entity id : view) {
                entity e(id, this);
                const auto& transform = e.get_component<transform_component>();
                auto& rb = e.get_component<rigid_body_component>();

                b2Body* body = (b2Body*)rb.runtime_body;
                b2Vec2 position;
                position.x = transform.translation.x;
                position.y = transform.translation.y;
                body->SetTransform(position, glm::radians(transform.rotation));
            }

            m_physics_world->Step(ts.count(), velocity_iterations, position_iterations);

            for (entt::entity id : view) {
                entity e(id, this);
                auto& transform = e.get_component<transform_component>();
                auto& rb = e.get_component<rigid_body_component>();

                b2Body* body = (b2Body*)rb.runtime_body;
                const auto& position = body->GetPosition();
                transform.translation.x = position.x;
                transform.translation.y = position.y;
                transform.rotation = glm::degrees(body->GetAngle());
            }
        }

        // Render
        runtime_camera* main_camera = nullptr;
        glm::mat4 camera_transform;
        {
            auto view = this->m_registry.view<transform_component, camera_component>();
            for (entt::entity id : view) {
                const auto& [camera_data, transform] =
                    this->m_registry.get<camera_component, transform_component>(id);

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

            render();

            renderer::end_scene();
        }
    }

    void scene::on_editor_update(timestep ts, const editor_camera& camera) {
        glm::mat4 view_projection = camera.get_view_projection_matrix();
        renderer::begin_scene(view_projection);

        renderer::draw_grid(camera);
        render();

        renderer::end_scene();
    }

    void scene::on_event(event& e) {
        event_dispatcher dispatcher(e);

        auto view = this->m_registry.view<native_script_component>();
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

        auto view = this->m_registry.view<camera_component>();
        for (entt::entity id : view) {
            auto& camera = view.get<camera_component>(id);
            camera.camera.set_render_target_size(width, height);
        }
    }

    void scene::for_each(const std::function<void(entity)>& callback) {
        m_registry.each(
            [callback, this](entt::entity id) mutable { view_iteration(id, callback); });
    }

    void scene::view_iteration(entt::entity id, const std::function<void(entity)>& callback) {
        entity e(id, this);
        callback(e);
    }

    void scene::render() {
        auto group = m_registry.group<transform_component>(entt::get<sprite_renderer_component>);
        for (auto entity : group) {
            auto [transform, sprite] =
                group.get<transform_component, sprite_renderer_component>(entity);

            if (sprite.texture) {
                renderer::draw_rotated_quad(transform.translation, transform.rotation,
                                            transform.scale, sprite.color, sprite.texture);
            } else {
                renderer::draw_rotated_quad(transform.translation, transform.rotation,
                                            transform.scale, sprite.color);
            }
        }
    }

    guid scene::get_guid(entity e) {
        auto& id_data = e.get_component<id_component>();
        return id_data.id;
    }
} // namespace sge
