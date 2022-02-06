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
#include "sge/script/script_engine.h"
#include "sge/script/script_helpers.h"
#include "sge/scene/scene.h"
#include "sge/scene/entity.h"
#include "sge/scene/components.h"
#include "sge/core/input.h"
namespace sge {
    struct component_callbacks_t {
        std::function<void*(entity)> get;
        std::function<bool(entity)> has;
    };

    static struct {
        std::unordered_map<void*, component_callbacks_t> component_callbacks;
    } internal_script_call_data;

    template <typename T>
    static void register_component_type(const std::string& managed_name) {
        class_name_t name;
        name.namespace_name = "SGE.Components";
        name.class_name = managed_name;

        void* scriptcore = script_engine::get_assembly(0);
        void* _class = script_engine::get_class(scriptcore, name);

        component_callbacks_t callbacks;
        callbacks.has = [](entity e) { return e.has_all<T>(); };
        callbacks.get = [](entity e) { return &e.get_component<T>(); };

        auto reflection_type = script_engine::to_reflection_type(_class);
        internal_script_call_data.component_callbacks.insert(
            std::make_pair(reflection_type, callbacks));
    }

    static void register_component_types() {
        register_component_type<tag_component>("TagComponent");
        register_component_type<transform_component>("TransformComponent");
        register_component_type<sprite_renderer_component>("SpriteRendererComponent");
        register_component_type<camera_component>("CameraComponent");
        register_component_type<rigid_body_component>("RigidBodyComponent");
        register_component_type<box_collider_component>("BoxColliderComponent");
    }

    static void verify_component_type_validity(void* reflection_type) {
        if (internal_script_call_data.component_callbacks.find(reflection_type) ==
            internal_script_call_data.component_callbacks.end()) {
            void* _class = script_engine::from_reflection_type(reflection_type);

            class_name_t name_data;
            script_engine::get_class_name(_class, name_data);

            throw std::runtime_error("managed type " + script_engine::get_string(name_data) +
                                     " is not registered as a component type!");
        }
    }

    namespace internal_script_calls {
        static uint32_t CreateEntity(void* name, void* _scene) {
            auto scene_ptr = (scene*)_scene;

            std::string native_name = script_engine::from_managed_string(name);
            return (uint32_t)scene_ptr->create_entity(native_name);
        }

        static uint32_t CreateEntityWithGUID(guid id, void* name, void* _scene) {
            auto scene_ptr = (scene*)_scene;

            std::string native_name = script_engine::from_managed_string(name);
            return (uint32_t)scene_ptr->create_entity(id, native_name);
        }

        static void DestroyEntity(uint32_t entityID, void* _scene) {
            auto scene_ptr = (scene*)_scene;
            entity e((entt::entity)entityID, scene_ptr);

            scene_ptr->destroy_entity(e);
        }

        static bool FindEntity(guid id, uint32_t* entityID, void* _scene) {
            auto scene_ptr = (scene*)_scene;

            entity found_entity = scene_ptr->find_guid(id);
            if (found_entity) {
                *entityID = (uint32_t)found_entity;
                return true;
            }

            return false;
        }

        static bool HasComponent(void* componentType, void* _entity) {
            verify_component_type_validity(componentType);
            entity e = script_helpers::get_entity_from_object(_entity);

            const auto& callbacks = internal_script_call_data.component_callbacks[componentType];
            return callbacks.has(e);
        }

        static void* GetComponent(void* componentType, void* _entity) {
            verify_component_type_validity(componentType);
            entity e = script_helpers::get_entity_from_object(_entity);

            const auto& callbacks = internal_script_call_data.component_callbacks[componentType];
            return callbacks.get(e);
        }

        static guid GetGUID(void* _entity) {
            entity e = script_helpers::get_entity_from_object(_entity);
            return e.get_guid();
        }

        static guid GenerateGUID() { return guid(); }

        static void SetTag(tag_component* component, void* tag) {
            component->tag = script_engine::from_managed_string(tag);
        }

        static void* GetTag(tag_component* component) {
            return script_engine::to_managed_string(component->tag);
        }

        static void GetTranslation(transform_component* component, glm::vec2* translation) {
            *translation = component->translation;
        }

        static void SetTranslation(transform_component* component, glm::vec2 translation) {
            component->translation = translation;
        }

        static float GetRotation(transform_component* component) { return component->rotation; }

        static void SetRotation(transform_component* component, float rotation) {
            component->rotation = rotation;
        }

        static void GetScale(transform_component* component, glm::vec2* scale) {
            *scale = component->scale;
        }

        static void SetScale(transform_component* component, glm::vec2 scale) {
            component->scale = scale;
        }

        static bool GetPrimary(camera_component* component) { return component->primary; }

        static void SetPrimary(camera_component* component, bool primary) {
            component->primary = primary;
        }

        static projection_type GetProjectionType(camera_component* component) {
            return component->camera.get_projection_type();
        }

        static void SetProjectionType(camera_component* component, projection_type type) {
            component->camera.set_projection_type(type);
        }

        static float GetViewSize(camera_component* component) {
            return component->camera.get_orthographic_size();
        }

        static void SetViewSize(camera_component* component, float view_size) {
            component->camera.set_orthographic_size(view_size);
        }

        static float GetFOV(camera_component* component) {
            return component->camera.get_vertical_fov();
        }

        static void SetFOV(camera_component* component, float fov) {
            component->camera.set_vertical_fov(fov);
        }

        static void GetOrthographicClips(camera_component* component,
                                         runtime_camera::clips* clips) {
            *clips = component->camera.get_orthographic_clips();
        }

        static void SetOrthographicClips(camera_component* component, runtime_camera::clips clips) {
            component->camera.set_orthographic_clips(clips);
        }

        static void GetPerspectiveClips(camera_component* component, runtime_camera::clips* clips) {
            *clips = component->camera.get_perspective_clips();
        }

        static void SetPerspectiveClips(camera_component* component, runtime_camera::clips clips) {
            component->camera.set_perspective_clips(clips);
        }

        static void SetOrthographic(camera_component* component, float viewSize,
                                    runtime_camera::clips clips) {
            component->camera.set_orthographic(viewSize, clips.near, clips.far);
        }

        static void SetPerspective(camera_component* component, float fov,
                                   runtime_camera::clips clips) {
            component->camera.set_perspective(fov, clips.near, clips.far);
        }

        static rigid_body_component::body_type GetBodyType(rigid_body_component* rb) {
            return rb->type;
        }

        static void SetBodyType(rigid_body_component* rb, void* _entity,
                                rigid_body_component::body_type type) {
            rb->type = type;

            entity e = script_helpers::get_entity_from_object(_entity);
            e.get_scene()->update_physics_data(e);
        }

        static bool GetFixedRotation(rigid_body_component* rb) { return rb->fixed_rotation; }

        static void SetFixedRotation(rigid_body_component* rb, void* _entity, bool fixed_rotation) {
            rb->fixed_rotation = fixed_rotation;

            entity e = script_helpers::get_entity_from_object(_entity);
            e.get_scene()->update_physics_data(e);
        }

        static bool GetAngularVelocity(void* _entity, float* velocity) {
            entity e = script_helpers::get_entity_from_object(_entity);

            scene* _scene = e.get_scene();
            std::optional<float> value = _scene->get_angular_velocity(e);

            if (value.has_value()) {
                *velocity = value.value();
                return true;
            }

            return false;
        }

        static bool SetAngularVelocity(void* _entity, float velocity) {
            entity e = script_helpers::get_entity_from_object(_entity);

            scene* _scene = e.get_scene();
            return _scene->set_angular_velocity(e, velocity);
        }

        static bool AddForce(void* _entity, glm::vec2 force, bool wake) {
            entity e = script_helpers::get_entity_from_object(_entity);

            scene* _scene = e.get_scene();
            return _scene->add_force(e, force, wake);
        }

        static void GetSize(box_collider_component* bc, glm::vec2* size) { *size = bc->size; }

        static void SetSize(box_collider_component* bc, void* _entity, glm::vec2 size) {
            bc->size = size;

            entity e = script_helpers::get_entity_from_object(_entity);
            e.get_scene()->update_physics_data(e);
        }

        static float GetDensity(box_collider_component* bc) { return bc->density; }

        static void SetDensity(box_collider_component* bc, void* _entity, float density) {
            bc->density = density;

            entity e = script_helpers::get_entity_from_object(_entity);
            e.get_scene()->update_physics_data(e);
        }

        static float GetFriction(box_collider_component* bc) { return bc->friction; }

        static void SetFriction(box_collider_component* bc, void* _entity, float friction) {
            bc->friction = friction;

            entity e = script_helpers::get_entity_from_object(_entity);
            e.get_scene()->update_physics_data(e);
        }

        static float GetRestitution(box_collider_component* bc) { return bc->restitution; }

        static void SetRestitution(box_collider_component* bc, void* _entity, float restitution) {
            bc->restitution = restitution;

            entity e = script_helpers::get_entity_from_object(_entity);
            e.get_scene()->update_physics_data(e);
        }

        static float GetRestitutionThreashold(box_collider_component* bc) {
            return bc->restitution_threashold;
        }

        static void SetRestitutionThreashold(box_collider_component* bc, void* _entity,
                                             float threashold) {
            bc->restitution_threashold = threashold;

            entity e = script_helpers::get_entity_from_object(_entity);
            e.get_scene()->update_physics_data(e);
        }

        static void LogDebug(void* message) {
            std::string string = script_engine::from_managed_string(message);
            spdlog::debug(string);
        }

        static void LogInfo(void* message) {
            std::string string = script_engine::from_managed_string(message);
            spdlog::info(string);
        }

        static void LogWarn(void* message) {
            std::string string = script_engine::from_managed_string(message);
            spdlog::warn(string);
        }

        static void LogError(void* message) {
            std::string string = script_engine::from_managed_string(message);
            spdlog::error(string);
        }

        static bool GetKey(key_code key) { return input::get_key(key); }
        static bool GetMouseButton(mouse_button button) { return input::get_mouse_button(button); }

        static void GetMousePosition(glm::vec2* position) {
            *position = input::get_mouse_position();
        }

        static bool IsEventHandled(event* address) { return address->handled; }
        static void SetEventHandled(event* address, bool handled) { address->handled = handled; }

        static int32_t GetResizeWidth(window_resize_event* address) {
            return (int32_t)address->get_width();
        }

        static int32_t GetResizeHeight(window_resize_event* address) {
            return (int32_t)address->get_height();
        }

        static void GetPressedEventKey(key_pressed_event* address, key_code* key) {
            *key = address->get_key();
        }

        static int32_t GetRepeatCount(key_pressed_event* address) {
            return (int32_t)address->get_repeat_count();
        }

        static void GetReleasedEventKey(key_released_event* address, key_code* key) {
            *key = address->get_key();
        }

        static void GetTypedEventKey(key_typed_event* address, key_code* key) {
            *key = address->get_key();
        }

        static void GetEventMousePosition(mouse_moved_event* address, glm::vec2* position) {
            *position = address->get_position();
        }

        static void GetScrollOffset(mouse_scrolled_event* address, glm::vec2* position) {
            *position = address->get_offset();
        }

        static void GetEventMouseButton(mouse_button_event* address, mouse_button* button) {
            *button = address->get_button();
        }

        static bool GetMouseButtonReleased(mouse_button_event* address) {
            return address->get_released();
        }
    } // namespace internal_script_calls

    void script_engine::register_internal_script_calls() {
        register_component_types();

#define REGISTER_CALL(name)                                                                        \
    register_internal_call("SGE.InternalCalls::" #name, (void*)internal_script_calls::name)

        // scene
        REGISTER_CALL(CreateEntity);
        REGISTER_CALL(CreateEntityWithGUID);
        REGISTER_CALL(DestroyEntity);
        REGISTER_CALL(FindEntity);

        // entity
        REGISTER_CALL(HasComponent);
        REGISTER_CALL(GetComponent);
        REGISTER_CALL(GetGUID);

        // guid
        REGISTER_CALL(GenerateGUID);

        // tag component
        REGISTER_CALL(SetTag);
        REGISTER_CALL(GetTag);

        // transform component
        REGISTER_CALL(GetTranslation);
        REGISTER_CALL(SetTranslation);
        REGISTER_CALL(GetRotation);
        REGISTER_CALL(SetRotation);
        REGISTER_CALL(GetScale);
        REGISTER_CALL(SetScale);

        // camera component
        REGISTER_CALL(GetPrimary);
        REGISTER_CALL(SetPrimary);
        REGISTER_CALL(GetProjectionType);
        REGISTER_CALL(SetProjectionType);
        REGISTER_CALL(GetViewSize);
        REGISTER_CALL(SetViewSize);
        REGISTER_CALL(GetFOV);
        REGISTER_CALL(SetFOV);
        REGISTER_CALL(GetOrthographicClips);
        REGISTER_CALL(SetOrthographicClips);
        REGISTER_CALL(GetPerspectiveClips);
        REGISTER_CALL(SetPerspectiveClips);
        REGISTER_CALL(SetOrthographic);
        REGISTER_CALL(SetPerspective);

        // rigid body component
        REGISTER_CALL(GetBodyType);
        REGISTER_CALL(SetBodyType);
        REGISTER_CALL(GetFixedRotation);
        REGISTER_CALL(SetFixedRotation);
        REGISTER_CALL(GetAngularVelocity);
        REGISTER_CALL(SetAngularVelocity);
        REGISTER_CALL(AddForce);

        // box collider component
        REGISTER_CALL(GetSize);
        REGISTER_CALL(SetSize);
        REGISTER_CALL(GetDensity);
        REGISTER_CALL(SetDensity);
        REGISTER_CALL(GetFriction);
        REGISTER_CALL(SetFriction);
        REGISTER_CALL(GetRestitution);
        REGISTER_CALL(SetRestitution);
        REGISTER_CALL(GetRestitutionThreashold);
        REGISTER_CALL(SetRestitutionThreashold);

        // logger
        REGISTER_CALL(LogDebug);
        REGISTER_CALL(LogInfo);
        REGISTER_CALL(LogWarn);
        REGISTER_CALL(LogError);

        // input
        REGISTER_CALL(GetKey);
        REGISTER_CALL(GetMouseButton);
        REGISTER_CALL(GetMousePosition);

        // events
        REGISTER_CALL(IsEventHandled);
        REGISTER_CALL(SetEventHandled);
        REGISTER_CALL(GetResizeWidth);
        REGISTER_CALL(GetResizeHeight);
        REGISTER_CALL(GetPressedEventKey);
        REGISTER_CALL(GetRepeatCount);
        REGISTER_CALL(GetReleasedEventKey);
        REGISTER_CALL(GetTypedEventKey);
        REGISTER_CALL(GetEventMousePosition);
        REGISTER_CALL(GetScrollOffset);
        REGISTER_CALL(GetEventMouseButton);
        REGISTER_CALL(GetMouseButtonReleased);

#undef REGISTER_CALL
    }
} // namespace sge
