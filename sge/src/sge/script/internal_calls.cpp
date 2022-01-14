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
#include "sge/script/internal_calls.h"
#include "sge/script/script_engine.h"
#include "sge/scene/scene.h"
#include "sge/scene/entity.h"
#include "sge/scene/components.h"
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
        void* _class = script_engine::get_class(0, name);

        component_callbacks_t callbacks;
        callbacks.has = [](entity e) { return e.has_all<T>(); };

        callbacks.get = [_class](entity e) mutable {
            void* constructor = script_engine::get_method(_class, ".ctor");
            auto component = &e.get_component<T>();

            void* instance = script_engine::alloc_object(_class);
            script_engine::call_method(instance, constructor, &component);
            return instance;
        };

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
        static bool HasComponent(void* componentType, uint32_t entityID, void* _scene) {
            verify_component_type_validity(componentType);

            auto scene_ptr = (scene*)_scene;
            entity e((entt::entity)entityID, scene_ptr);

            const auto& callbacks = internal_script_call_data.component_callbacks[componentType];
            return callbacks.has(e);
        }

        static void* GetComponent(void* componentType, uint32_t entityID, void* _scene) {
            verify_component_type_validity(componentType);

            auto scene_ptr = (scene*)_scene;
            entity e((entt::entity)entityID, scene_ptr);

            const auto& callbacks = internal_script_call_data.component_callbacks[componentType];
            return callbacks.get(e);
        }

        static guid GetGUID(uint32_t entityID, void* _scene) {
            auto scene_ptr = (scene*)_scene;
            entity e((entt::entity)entityID, scene_ptr);
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

        static void SetBodyType(rigid_body_component* rb, rigid_body_component::body_type type) {
            rb->type = type;
        }

        static bool GetFixedRotation(rigid_body_component* rb) { return rb->fixed_rotation; }

        static void SetFixedRotation(rigid_body_component* rb, bool fixed_rotation) {
            rb->fixed_rotation = fixed_rotation;
        }

        static void GetSize(box_collider_component* bc, glm::vec2* size) { *size = bc->size; }
        static void SetSize(box_collider_component* bc, glm::vec2 size) { bc->size = size; }

        static float GetDensity(box_collider_component* bc) { return bc->density; }
        static void SetDensity(box_collider_component* bc, float density) { bc->density = density; }

        static float GetFriction(box_collider_component* bc) { return bc->friction; }

        static void SetFriction(box_collider_component* bc, float friction) {
            bc->friction = friction;
        }

        static float GetRestitution(box_collider_component* bc) { return bc->restitution; }
        
        static void SetRestitution(box_collider_component* bc, float restitution) {
            bc->restitution = restitution;
        }

        static float GetRestitutionThreashold(box_collider_component* bc) {
            return bc->restitution_threashold;
        }

        static void SetRestitutionThreashold(box_collider_component* bc, float threashold) {
            bc->restitution_threashold = threashold;
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
    } // namespace internal_script_calls

    void register_internal_script_calls() {
        register_component_types();

#define REGISTER_CALL(name)                                                                        \
    script_engine::register_internal_call("SGE.InternalCalls::" #name,                             \
                                          (void*)internal_script_calls::name)

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

#undef REGISTER_CALL
    }
} // namespace sge
