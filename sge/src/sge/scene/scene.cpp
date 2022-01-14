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
#include "sge/script/script_engine.h"
#include "sge/script/garbage_collector.h"

#include <box2d/b2_world.h>
#include <box2d/b2_body.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_circle_shape.h>
#include <box2d/b2_contact.h>

namespace sge {

    struct scene_physics_data {
        b2World* world;
        std::unique_ptr<b2ContactListener> listener;
    };

    class scene_contact_listener : public b2ContactListener {
    public:
        static std::unique_ptr<b2ContactListener> create(ref<scene> _scene) {
            auto instance = new scene_contact_listener(_scene.raw());
            return std::unique_ptr<b2ContactListener>(instance);
        }

        virtual void BeginContact(b2Contact* contact) override {
            b2Fixture* fixture = contact->GetFixtureA();
            uintptr_t user_data = fixture->GetUserData().pointer;
            entity entity_a((entt::entity)(uint32_t)user_data, m_scene);

            fixture = contact->GetFixtureB();
            user_data = fixture->GetUserData().pointer;
            entity entity_b((entt::entity)(uint32_t)user_data, m_scene);

            if (entity_a.has_all<native_script_component>()) {
                auto& nsc = entity_a.get_component<native_script_component>();
                if (nsc.script != nullptr) {
                    nsc.script->on_collision(entity_b);
                }
            }

            if (entity_b.has_all<native_script_component>()) {
                auto& nsc = entity_b.get_component<native_script_component>();
                if (nsc.script != nullptr) {
                    nsc.script->on_collision(entity_a);
                }
            }
        }

    private:
        scene_contact_listener(scene* _scene) { m_scene = _scene; }

        scene* m_scene;
    };

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
        {
            auto view = m_registry.view<native_script_component>();
            for (entt::entity id : view) {
                entity e(id, this);
                auto& nsc = e.get_component<native_script_component>();
                if (nsc.script != nullptr) {
                    nsc.destroy(&nsc);
                }
            }
        }

        {
            auto view = m_registry.view<script_component>();
            for (entt::entity id : view) {
                entity e(id, this);
                remove_script(e);
            }
        }

        if (m_physics_data != nullptr) {
            delete m_physics_data->world;
            delete m_physics_data;
        }
    }

    entity scene::create_entity(const std::string& name) {
        // default constructor generates
        guid id;

        return create_entity(id, name);
    }

    entity scene::create_entity(guid id, const std::string& name) {
        entity e(m_registry.create(), this);

        e.add_component<id_component>().id = id;
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

        if (e.has_all<script_component>()) {
            remove_script(e);
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

        {
            auto view = m_registry.view<script_component>();
            for (entt::entity id : view) {
                entity e(id, this);
                remove_script(e);
            }
        }

        m_registry.clear();
    }

    void scene::set_script(entity e, void* _class) {
        if (!e.has_all<script_component>()) {
            e.add_component<script_component>();
        }

        remove_script(e);
        e.get_component<script_component>()._class = _class;
    }

    void scene::reset_script(entity e) {
        if (e.has_all<script_component>()) {
            remove_script(e);
            e.remove_component<script_component>();
        }
    }

    template <typename T>
    static T copy_component(const T& src) {
        return src;
    }

    template <>
    script_component copy_component<script_component>(const script_component& src) {
        script_component dst;
        dst._class = src._class;

        void* instance = garbage_collector::get_ref_data(src.gc_handle);
        dst.gc_handle = garbage_collector::create_ref(instance);

        return dst;
    }

    template <typename T>
    static void copy_component_type(entt::registry& dst, entt::registry& src,
                                    const std::unordered_map<guid, entt::entity>& entity_map) {
        auto view = src.view<T>();
        for (entt::entity e : view) {
            guid id = src.get<id_component>(e).id;
            if (entity_map.find(id) == entity_map.end()) {
                throw std::runtime_error("entity registries do not match!");
            }

            entt::entity new_entity = entity_map.at(id);
            T& data = src.get<T>(e);
            dst.emplace_or_replace<T>(new_entity, copy_component(data));
        }
    }

    ref<scene> scene::copy() {
        auto new_scene = ref<scene>::create();

        entt::registry& dst_registry = new_scene->m_registry;
        std::unordered_map<guid, entt::entity> entity_map;

        {
            auto view = m_registry.view<id_component>();
            for (entt::entity id : view) {
                entity original(id, this);
                guid entity_id = original.get_component<id_component>().id;
                const std::string& tag = original.get_component<tag_component>().tag;

                entity new_entity = new_scene->create_entity(entity_id, tag);
                entity_map.insert(std::make_pair(entity_id, (entt::entity)new_entity));
            }
        }

        copy_component_type<transform_component>(dst_registry, m_registry, entity_map);
        copy_component_type<camera_component>(dst_registry, m_registry, entity_map);
        copy_component_type<sprite_renderer_component>(dst_registry, m_registry, entity_map);
        copy_component_type<native_script_component>(dst_registry, m_registry, entity_map);
        copy_component_type<rigid_body_component>(dst_registry, m_registry, entity_map);
        copy_component_type<box_collider_component>(dst_registry, m_registry, entity_map);
        copy_component_type<script_component>(dst_registry, m_registry, entity_map);

        new_scene->set_viewport_size(m_viewport_width, m_viewport_height);
        return new_scene;
    }

    void scene::on_start() {
        // Initialize the box2d physics engine
        {
            m_physics_data = new scene_physics_data;
            m_physics_data->world = new b2World(b2Vec2(0.f, -9.8f));
            m_physics_data->listener = scene_contact_listener::create(this);
            m_physics_data->world->SetContactListener(m_physics_data->listener.get());

            auto view = m_registry.view<rigid_body_component>();
            for (auto id : view) {
                entity e(id, this);
                auto& transform = e.get_component<transform_component>();
                auto& rb = e.get_component<rigid_body_component>();

                b2BodyDef body_def;
                body_def.type = rigid_body_type_to_box2d_body(rb.type);
                body_def.position.Set(transform.translation.x, transform.translation.y);
                body_def.angle = glm::radians(transform.rotation);
                b2Body* body = m_physics_data->world->CreateBody(&body_def);
                body->SetFixedRotation(rb.fixed_rotation);
                rb.runtime_body = body;

                if (e.has_all<box_collider_component>()) {
                    auto& bc = e.get_component<box_collider_component>();

                    b2PolygonShape box_shape;
                    box_shape.SetAsBox(bc.size.x * transform.scale.x,
                                       bc.size.y * transform.scale.y);

                    b2FixtureDef fixture_def;
                    fixture_def.shape = &box_shape;
                    fixture_def.density = bc.density;
                    fixture_def.friction = bc.friction;
                    fixture_def.restitution = bc.restitution;
                    fixture_def.restitutionThreshold = bc.restitution_threashold;
                    fixture_def.userData.pointer = (uintptr_t)(uint32_t)e;

                    auto fixture = body->CreateFixture(&fixture_def);
                    bc.runtime_fixture = fixture;
                }
            }
        }

        // Call OnStart method, if it exists
        {
            auto view = m_registry.view<script_component>();
            for (entt::entity id : view) {
                entity e(id, this);
                verify_script(e);

                auto& sc = e.get_component<script_component>();
                if (sc._class == nullptr) {
                    continue;
                }

                void* OnStart = script_engine::get_method(sc._class, "OnStart()");
                if (OnStart != nullptr) {
                    void* instance = garbage_collector::get_ref_data(sc.gc_handle);
                    script_engine::call_method(instance, OnStart);
                }
            }
        }
    }

    void scene::on_stop() {
        // Call OnStop method, if it exists
        {
            auto view = m_registry.view<script_component>();
            for (entt::entity id : view) {
                entity e(id, this);
                verify_script(e);

                auto& sc = e.get_component<script_component>();
                if (sc._class == nullptr) {
                    continue;
                }

                void* OnStop = script_engine::get_method(sc._class, "OnStop()");
                if (OnStop != nullptr) {
                    void* instance = garbage_collector::get_ref_data(sc.gc_handle);
                    script_engine::call_method(instance, OnStop);
                }
            }
        }

        // Delete physics data
        delete m_physics_data->world;
        delete m_physics_data;
        m_physics_data = nullptr;
    }

    void scene::on_runtime_update(timestep ts) {
        // Native Scripts
        {
            auto view = m_registry.view<native_script_component>();
            for (auto id : view) {
                entity entity(id, this);
                auto& nsc = entity.get_component<native_script_component>();

                if (nsc.script == nullptr && nsc.instantiate != nullptr) {
                    nsc.instantiate(&nsc, entity);
                }

                if (nsc.script != nullptr) {
                    nsc.script->on_update(ts);
                }
            }
        }

        // Managed Scripts
        {
            auto view = m_registry.view<script_component>();
            for (auto id : view) {
                entity e(id, this);
                verify_script(e);

                auto& sc = e.get_component<script_component>();
                if (sc._class == nullptr) {
                    continue;
                }

                void* OnUpdate = script_engine::get_method(sc._class, "OnUpdate(Timestep)");
                if (OnUpdate != nullptr) {
                    void* instance = garbage_collector::get_ref_data(sc.gc_handle);

                    double timestep_data = ts.count();
                    script_engine::call_method(instance, OnUpdate, &timestep_data);
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

            m_physics_data->world->Step(ts.count(), velocity_iterations, position_iterations);

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
        {
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

            glm::mat4 view_projection;
            if (main_camera != nullptr) {
                glm::mat4 projection = main_camera->get_projection();
                view_projection = projection * glm::inverse(camera_transform);
            } else {
                static constexpr float default_view_size = 10.f;
                float aspect_ratio = (float)m_viewport_width / (float)m_viewport_height;

                float left = -default_view_size * aspect_ratio / 2.f;
                float right = default_view_size * aspect_ratio / 2.f;
                float bottom = -default_view_size / 2.f;
                float top = default_view_size / 2.f;

                view_projection = glm::ortho(left, right, bottom, top, -1.f, 1.f);
            }

            renderer::begin_scene(view_projection);
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

    void scene::verify_script(entity e) {
        auto& sc = e.get_component<script_component>();
        if (sc._class != nullptr && sc.gc_handle == 0) {
            void* instance = script_engine::alloc_object(sc._class);

            if (script_engine::get_method(sc._class, ".ctor") != nullptr) {
                void* constructor = script_engine::get_method(sc._class, ".ctor()");
                if (constructor != nullptr) {
                    script_engine::call_method(instance, constructor);
                } else {
                    class_name_t name_data;
                    script_engine::get_class_name(sc._class, name_data);
                    std::string full_name = script_engine::get_string(name_data);

                    throw std::runtime_error("could not find a suitable constructor for script: " +
                                             full_name);
                }
            } else {
                script_engine::init_object(instance);
            }

            sc.gc_handle = garbage_collector::create_ref(instance);
        }
    }

    void scene::remove_script(entity e) {
        auto& sc = e.get_component<script_component>();
        if (sc._class != nullptr) {
            if (sc.gc_handle != 0) {
                garbage_collector::destroy_ref(sc.gc_handle);
                sc.gc_handle = 0;
            }

            sc._class = nullptr;
        }
    }

    guid scene::get_guid(entity e) {
        auto& id_data = e.get_component<id_component>();
        return id_data.id;
    }
} // namespace sge
