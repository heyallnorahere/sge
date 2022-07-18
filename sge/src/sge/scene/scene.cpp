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
#include "sge/script/script_helpers.h"
#include "sge/script/garbage_collector.h"

#include <box2d/b2_world.h>
#include <box2d/b2_body.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_circle_shape.h>
#include <box2d/b2_contact.h>

namespace sge {
    enum class collider_type {
        box,
    };

    struct entity_physics_data {
        std::optional<collider_type> _collider_type;
        std::optional<glm::vec2> current_box_size;

        b2Fixture* fixture;
        b2Body* body;
    };

    struct scene_physics_data {
        std::unordered_map<entt::entity, entity_physics_data> bodies;

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

            // native scripts
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

            // managed scripts
            std::string event_name = "OnCollision(Entity)";
            if (entity_a.has_all<script_component>()) {
                m_scene->verify_script(entity_a);

                auto& sc = entity_a.get_component<script_component>();
                if (sc._class != nullptr && sc.enabled) {
                    void* OnCollision = script_engine::get_method(sc._class, event_name);

                    if (OnCollision != nullptr) {
                        void* instance = sc.instance->get();

                        void* param = script_helpers::create_entity_object(entity_b);
                        script_engine::call_method(instance, OnCollision, param);
                    }
                }
            }

            if (entity_b.has_all<script_component>()) {
                m_scene->verify_script(entity_b);

                auto& sc = entity_b.get_component<script_component>();
                if (sc._class != nullptr && sc.enabled) {
                    void* OnCollision = script_engine::get_method(sc._class, event_name);

                    if (OnCollision != nullptr) {
                        void* instance = sc.instance->get();

                        void* param = script_helpers::create_entity_object(entity_a);
                        script_engine::call_method(instance, OnCollision, param);
                    }
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

    entity scene::clone_entity(entity src, const std::string& name) {
        auto dst = create_entity();

        for (auto&& [id_type, storage] : m_registry.storage()) {
            if (!storage.contains(src)) {
                continue;
            }

            auto type = entt::resolve(storage.type());
            if (!type) {
                continue;
            }

            using namespace entt::literals;
            auto clone = type.func("clone"_hs);
            if (!clone) {
                continue;
            }

            auto raw = storage.get(src);
            if (!clone.invoke({}, entt::forward_as_meta(src), entt::forward_as_meta(dst), raw)) {
                throw std::runtime_error("could not clone component!");
            }
        }

        auto& dst_tag = dst.get_component<tag_component>();
        if (!name.empty()) {
            dst_tag.tag = name;
        } else {
            const auto& src_tag = src.get_component<tag_component>();
            dst_tag.tag = src_tag.tag + " - Copy";
        }

        return dst;
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
        recalculate_render_order();
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
        for (std::string& name : m_collision_category_names) {
            name.clear();
        }
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

    void scene::verify_script(entity e) {
        if (!e.has_all<script_component>()) {
            return;
        }

        auto& sc = e.get_component<script_component>();
        sc.verify_script(e);
    }

    void scene::update_physics_data(entity e) {
        if (e.has_all<rigid_body_component>()) {
            if (m_physics_data->bodies.find(e) == m_physics_data->bodies.end()) {
                m_physics_data->bodies.insert(
                    std::make_pair((entt::entity)e, entity_physics_data()));
            }
            auto& data = m_physics_data->bodies[e];

            auto& rb = e.get_component<rigid_body_component>();
            auto& transform = e.get_component<transform_component>();

            if (data.body == nullptr) {
                b2BodyDef body_def;
                body_def.type = rigid_body_type_to_box2d_body(rb.type);
                body_def.position.Set(transform.translation.x, transform.translation.y);
                body_def.angle = glm::radians(transform.rotation);
                body_def.fixedRotation = rb.fixed_rotation;

                data.body = m_physics_data->world->CreateBody(&body_def);
            } else {
                b2Vec2 position;
                position.x = transform.translation.x;
                position.y = transform.translation.y;

                data.body->SetTransform(position, glm::radians(transform.rotation));
                data.body->SetFixedRotation(rb.fixed_rotation);
                data.body->SetType(rigid_body_type_to_box2d_body(rb.type));
            }

            std::optional<collider_type> type;
            if (e.has_all<box_collider_component>()) {
                type = collider_type::box;
            }

            if (type.has_value()) {
                switch (type.value()) {
                case collider_type::box: {
                    auto& bc = e.get_component<box_collider_component>();
                    glm::vec2 collider_size = bc.size * transform.scale;

                    bool create_fixture =
                        (data.fixture == nullptr) || (data._collider_type != collider_type::box);
                    if (data.current_box_size.has_value()) {
                        create_fixture |=
                            (glm::length(data.current_box_size.value() - collider_size) > 0.0001f);
                    }

                    if (create_fixture) {
                        if (data.fixture != nullptr) {
                            data.body->DestroyFixture(data.fixture);
                        }

                        b2PolygonShape shape;
                        shape.SetAsBox(collider_size.x, collider_size.y);

                        b2FixtureDef fixture_def;
                        fixture_def.shape = &shape;
                        fixture_def.density = bc.density;
                        fixture_def.friction = bc.friction;
                        fixture_def.restitution = bc.restitution;
                        fixture_def.restitutionThreshold = bc.restitution_threashold;
                        fixture_def.filter.categoryBits = rb.filter_category;
                        fixture_def.filter.maskBits = rb.filter_mask;
                        fixture_def.userData.pointer = (uintptr_t)(uint32_t)e;

                        data.fixture = data.body->CreateFixture(&fixture_def);
                        data.current_box_size = collider_size;
                        data._collider_type = collider_type::box;
                    } else {
                        if (fabs(data.fixture->GetDensity() - bc.density) > 0.0001f) {
                            data.fixture->SetDensity(bc.density);
                            data.body->ResetMassData();
                        }

                        data.fixture->SetFriction(bc.friction);
                        data.fixture->SetRestitution(bc.restitution);
                        data.fixture->SetRestitutionThreshold(bc.restitution_threashold);

                        b2Filter filter;
                        filter.categoryBits = rb.filter_category;
                        filter.maskBits = rb.filter_mask;
                        data.fixture->SetFilterData(filter);
                    }
                } break;
                default:
                    throw std::runtime_error("invalid collider type!");
                }
            } else if (data.fixture != nullptr) {
                data.body->DestroyFixture(data.fixture);
                data.fixture = nullptr;
            }
        } else if (m_physics_data->bodies.find(e) != m_physics_data->bodies.end()) {
            b2Body* body = m_physics_data->bodies[e].body;
            if (body != nullptr) {
                m_physics_data->world->DestroyBody(body);
            }

            m_physics_data->bodies.erase(e);
        }
    }

    static guid get_shader_guid(ref<shader> _shader) {
        if (!_shader) {
            return 0;
        }

        return _shader->id;
    }

    void scene::recalculate_render_order() {
        m_render_order.clear();

        auto group = m_registry.group<transform_component>(entt::get<sprite_renderer_component>);
        if (group.empty()) {
            return;
        }

        std::map<int32_t, std::unordered_map<guid, size_t>> z_layers;
        std::optional<int32_t> upper_z_bound;

        std::vector<entity> entities;
        for (auto _entity : group) {
            const auto& [transform, sprite] =
                group.get<transform_component, sprite_renderer_component>(_entity);

            if (!upper_z_bound.has_value() || transform.z_layer > upper_z_bound.value()) {
                upper_z_bound = transform.z_layer;
            }

            auto& shader_indices = z_layers[transform.z_layer];
            guid id = get_shader_guid(sprite._shader);

            if (shader_indices.find(id) == shader_indices.end()) {
                shader_indices.insert(std::make_pair(id, 0));
            }

            entities.push_back(entity(_entity, this));
        }

        for (auto _entity : entities) {
            const auto& transform = _entity.get_component<transform_component>();
            const auto& sprite = _entity.get_component<sprite_renderer_component>();

            guid id = get_shader_guid(sprite._shader);
            size_t insert_index = z_layers[transform.z_layer][id];

            {
                auto it = m_render_order.begin();
                std::advance(it, insert_index);
                m_render_order.insert(it, _entity);
            }

            auto main_it = z_layers.find(transform.z_layer);
            auto sub_map = &main_it->second;

            for (auto sub_it = sub_map->find(id); sub_it != sub_map->end(); sub_it++) {
                sub_it->second++;
            }

            for (main_it++; main_it != z_layers.end(); main_it++) {
                for (auto& [shader_id, index] : main_it->second) {
                    index++;
                }
            }
        }
    }

    bool scene::apply_force(entity e, glm::vec2 force, glm::vec2 point, bool wake) {
        if (!e.has_all<rigid_body_component>()) {
            return false;
        }

        // verify that the given entity has a b2Body attached
        update_physics_data(e);

        // apply the force
        b2Vec2 b2_force(force.x, force.y);
        b2Vec2 b2_point(point.x, point.y);
        m_physics_data->bodies[e].body->ApplyForce(b2_force, b2_point, wake);

        return true;
    }

    bool scene::apply_force(entity e, glm::vec2 force, bool wake) {
        if (!e.has_all<rigid_body_component>()) {
            return false;
        }

        // verify that the given entity has a b2Body attached
        update_physics_data(e);

        // apply the force
        b2Vec2 b2_force(force.x, force.y);
        m_physics_data->bodies[e].body->ApplyForceToCenter(b2_force, wake);

        return true;
    }

    bool scene::apply_linear_impulse(entity e, glm::vec2 impulse, glm::vec2 point, bool wake) {
        if (!e.has_all<rigid_body_component>()) {
            return false;
        }

        // verify that the given entity has a b2Body attached
        update_physics_data(e);

        // apply the impulse
        b2Vec2 b2_impulse(impulse.x, impulse.y);
        b2Vec2 b2_point(point.x, point.y);
        m_physics_data->bodies[e].body->ApplyLinearImpulse(b2_impulse, b2_point, wake);

        return true;
    }

    bool scene::apply_linear_impulse(entity e, glm::vec2 impulse, bool wake) {
        if (!e.has_all<rigid_body_component>()) {
            return false;
        }

        // verify that the given entity has a b2Body attached
        update_physics_data(e);

        // apply the impulse
        b2Vec2 b2_impulse(impulse.x, impulse.y);
        m_physics_data->bodies[e].body->ApplyLinearImpulseToCenter(b2_impulse, wake);

        return true;
    }

    bool scene::apply_torque(entity e, float torque, bool wake) {
        if (!e.has_all<rigid_body_component>()) {
            return false;
        }

        // verify that the given entity has a b2Body attached
        update_physics_data(e);

        // apply the torque
        m_physics_data->bodies[e].body->ApplyTorque(torque, wake);
        return true;
    }

    std::optional<glm::vec2> scene::get_velocity(entity e) {
        std::optional<glm::vec2> velocity;

        if (e.has_all<rigid_body_component>()) {
            if (m_physics_data->bodies.find(e) != m_physics_data->bodies.end()) {
                b2Body* body = m_physics_data->bodies[e].body;

                const auto& linear_velocity = body->GetLinearVelocity();
                velocity = glm::vec2(linear_velocity.x, linear_velocity.y);
            }
        }

        return velocity;
    }

    bool scene::set_velocity(entity e, glm::vec2 velocity) {
        if (e.has_all<rigid_body_component>()) {
            if (m_physics_data->bodies.find(e) != m_physics_data->bodies.end()) {
                b2Body* body = m_physics_data->bodies[e].body;
                body->SetLinearVelocity(b2Vec2(velocity.x, velocity.y));

                return true;
            }
        }

        return false;
    }

    std::optional<float> scene::get_angular_velocity(entity e) {
        std::optional<float> velocity;

        if (e.has_all<rigid_body_component>()) {
            if (m_physics_data->bodies.find(e) != m_physics_data->bodies.end()) {
                b2Body* body = m_physics_data->bodies[e].body;
                velocity = body->GetAngularVelocity();
            }
        }

        return velocity;
    }

    bool scene::set_angular_velocity(entity e, float velocity) {
        if (e.has_all<rigid_body_component>()) {
            if (m_physics_data->bodies.find(e) != m_physics_data->bodies.end()) {
                b2Body* body = m_physics_data->bodies[e].body;
                body->SetAngularVelocity(velocity);

                return true;
            }
        }

        return false;
    }

    entity scene::find_guid(guid id) {
        entity found;

        auto view = m_registry.view<id_component>();
        for (entt::entity entity_id : view) {
            entity current(entity_id, this);

            guid current_id = current.get_component<id_component>().id;
            if (id == current_id) {
                found = current;
                break;
            }
        }

        return found;
    }

    ref<scene> scene::copy() {
        auto new_scene = ref<scene>::create();
        new_scene->m_collision_category_names = m_collision_category_names;

        // Map from the entt entity id in the old scene to the new scene
        std::unordered_map<entt::entity, entt::entity> entity_map;

        // Create corresponding entities (without components) in the new scene. Make sure guid and
        // tag match.  Also store map from id to new entity in entity map.
        {
            auto view = m_registry.view<id_component, tag_component>();
            for (entt::entity id : view) {
                entity original(id, this);
                guid entity_id = original.get_guid();
                const std::string& tag = original.get_component<tag_component>().tag;

                entity new_entity = new_scene->create_entity(entity_id, tag);
                entity_map.insert(std::make_pair(id, (entt::entity)new_entity));
            }
        }

        // Loop over every component type in source scene
        for (auto&& [id_type, storage] : m_registry.storage()) {
            using namespace entt::literals;

            // Only iterate over the entities in the storage pool if that type supports cloning.
            auto type = entt::resolve(storage.type());
            if (!type) {
                continue;
            }

            if (auto clone = type.func("clone"_hs); clone) {
                for (auto&& e : storage) {
                    auto srce = entity{ e, this };
                    auto dste = entity{ entity_map.at(e), new_scene.raw() };

                    auto raw = storage.get(e);
                    if (!clone.invoke({}, entt::forward_as_meta(srce), entt::forward_as_meta(dste),
                                      raw)) {
                        throw std::runtime_error("error cloning component!");
                    }
                }
            }
        }

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
                    void* instance = sc.instance->get();
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
                    void* instance = sc.instance->get();
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
                if (sc._class == nullptr || !sc.enabled) {
                    continue;
                }

                void* OnUpdate = script_engine::get_method(sc._class, "OnUpdate(Timestep)");
                if (OnUpdate != nullptr) {
                    void* instance = sc.instance->get();

                    double timestep_data = ts.count();
                    script_engine::call_method(instance, OnUpdate, &timestep_data);
                }
            }
        }

        // Physics
        {
            // update physics data for every entity in the scene
            for_each([this](entity e) { update_physics_data(e); });

            // update physics world
            static constexpr int32_t velocity_iterations = 6;
            static constexpr int32_t position_iterations = 2;
            m_physics_data->world->Step(ts.count(), velocity_iterations, position_iterations);

            // sync position data
            auto view = m_registry.view<transform_component, rigid_body_component>();
            for (entt::entity id : view) {
                entity e(id, this);
                auto& transform = e.get_component<transform_component>();

                if (m_physics_data->bodies.find(e) == m_physics_data->bodies.end()) {
                    continue;
                }

                b2Body* body = m_physics_data->bodies[e].body;
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

    static void create_event_class_map(std::unordered_map<event_id, void*>& map) {
        static const std::unordered_map<event_id, std::string> names = {
            { event_id::window_close, "WindowClose" },
            { event_id::window_resize, "WindowResize" },
            { event_id::key_pressed, "KeyPressed" },
            { event_id::key_released, "KeyReleased" },
            { event_id::key_typed, "KeyTyped" },
            { event_id::mouse_moved, "MouseMoved" },
            { event_id::mouse_scrolled, "MouseScrolled" },
            { event_id::mouse_button, "MouseButton" }
        };

        class_name_t name_data;
        name_data.namespace_name = "SGE.Events";

        void* core = script_engine::get_assembly(0);
        for (const auto& [id, name] : names) {
            name_data.class_name = name + "Event";
            void* _class = script_engine::get_class(core, name_data);

            if (_class != nullptr) {
                map.insert(std::make_pair(id, _class));
            }
        }
    }

    void scene::on_event(event& e) {
        // native scripts
        {
            auto view = m_registry.view<native_script_component>();
            for (auto entt_entity : view) {
                if (e.handled) {
                    break;
                }

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

        // managed scripts
        {
            ref<object_ref> event_handle;

            auto view = m_registry.view<script_component>();
            for (entt::entity id : view) {
                if (e.handled) {
                    break;
                }

                entity current(id, this);
                auto& sc = current.get_component<script_component>();
                if (sc._class == nullptr || !sc.enabled) {
                    continue;
                }

                static const std::string event_name = "OnEvent(Event)";
                void* OnEvent = script_engine::get_method(sc._class, event_name);
                if (OnEvent == nullptr) {
                    continue;
                }

                if (!event_handle) {
                    void* event_instance = script_helpers::create_event_object(e);
                    if (event_instance == nullptr) {
                        break;
                    }

                    event_handle = object_ref::from_object(event_instance);
                }

                verify_script(current);
                void* script_instance = sc.instance->get();

                void* event_instance = event_handle->get();
                script_engine::call_method(script_instance, OnEvent, event_instance);
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
        for (auto _entity : m_render_order) {
            const auto& transform = _entity.get_component<transform_component>();
            const auto& sprite = _entity.get_component<sprite_renderer_component>();

            auto _shader = sprite._shader;
            if (!_shader) {
                auto& library = renderer::get_shader_library();
                _shader = library.get("default");
            }

            renderer::set_shader(_shader);
            if (sprite.texture) {
                renderer::draw_rotated_quad(transform.translation, transform.rotation,
                                            transform.scale, sprite.color, sprite.texture);
            } else {
                renderer::draw_rotated_quad(transform.translation, transform.rotation,
                                            transform.scale, sprite.color);
            }
        }
    }

    void scene::remove_script(entity e, void* component) {
        script_component* sc;
        if (component != nullptr) {
            sc = (script_component*)component;
        } else {
            sc = &e.get_component<script_component>();
        }

        sc->remove_script();
    }

    guid scene::get_guid(entity e) {
        auto& id_data = e.get_component<id_component>();
        return id_data.id;
    }
} // namespace sge
