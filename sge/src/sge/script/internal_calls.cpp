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
#include "sge/script/garbage_collector.h"
#include "sge/scene/scene.h"
#include "sge/scene/entity.h"
#include "sge/scene/components.h"
#include "sge/core/input.h"
namespace sge {
    struct component_callbacks_t {
        std::function<void*(entity)> add, get;
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
        callbacks.add = [](entity e) { return &e.add_component<T>(); };
        callbacks.has = [](entity e) { return e.has_all<T>(); };
        callbacks.get = [](entity e) { return &e.get_component<T>(); };

        internal_script_call_data.component_callbacks.insert(std::make_pair(_class, callbacks));
    }

    void script_engine::register_component_types() {
        register_component_type<tag_component>("TagComponent");
        register_component_type<transform_component>("TransformComponent");
        register_component_type<sprite_renderer_component>("SpriteRendererComponent");
        register_component_type<camera_component>("CameraComponent");
        register_component_type<rigid_body_component>("RigidBodyComponent");
        register_component_type<box_collider_component>("BoxColliderComponent");
        register_component_type<script_component>("ScriptComponent");
    }

    static void verify_component_type_validity(void* reflection_type) {
        void* _class = script_engine::from_reflection_type(reflection_type);
        if (internal_script_call_data.component_callbacks.find(_class) ==
            internal_script_call_data.component_callbacks.end()) {

            class_name_t name_data;
            script_engine::get_class_name(_class, name_data);

            throw std::runtime_error("managed type " + script_engine::get_string(name_data) +
                                     " is not registered as a component type!");
        }
    }

    static const component_callbacks_t& get_component_callbacks(void* component_type) {
        void* _class = script_engine::from_reflection_type(component_type);
        return internal_script_call_data.component_callbacks[_class];
    }

    namespace internal_script_calls {
        static uint32_t CreateEntity(void* name, scene* _scene) {
            std::string native_name = script_engine::from_managed_string(name);
            return (uint32_t)_scene->create_entity(native_name);
        }

        static uint32_t CreateEntityWithGUID(guid id, void* name, scene* _scene) {
            std::string native_name = script_engine::from_managed_string(name);
            return (uint32_t)_scene->create_entity(id, native_name);
        }

        static uint32_t CloneEntity(uint32_t entityID, void* name, scene* _scene) {
            entity src((entt::entity)entityID, _scene);

            std::string native_name = script_engine::from_managed_string(name);
            return (uint32_t)_scene->clone_entity(src, native_name);
        }

        static void DestroyEntity(uint32_t entityID, scene* _scene) {
            entity e((entt::entity)entityID, _scene);
            _scene->destroy_entity(e);
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

        static void* GetCollisionCategoryName(scene* _scene, int32_t index) {
            std::string name = _scene->collision_category_name(index);
            return script_engine::to_managed_string(name);
        }

        static void* AddComponent(void* componentType, void* _entity) {
            verify_component_type_validity(componentType);
            entity e = script_helpers::get_entity_from_object(_entity);

            const auto& callbacks = get_component_callbacks(componentType);
            return callbacks.add(e);
        }

        static bool HasComponent(void* componentType, void* _entity) {
            verify_component_type_validity(componentType);
            entity e = script_helpers::get_entity_from_object(_entity);

            const auto& callbacks = get_component_callbacks(componentType);
            return callbacks.has(e);
        }

        static void* GetComponent(void* componentType, void* _entity) {
            verify_component_type_validity(componentType);
            entity e = script_helpers::get_entity_from_object(_entity);

            const auto& callbacks = get_component_callbacks(componentType);
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

        static void GetColor(sprite_renderer_component* component, glm::vec4* color) {
            *color = component->color;
        }

        static void SetColor(sprite_renderer_component* component, glm::vec4 color) {
            component->color = color;
        }

        static void GetTexture(sprite_renderer_component* component, texture_2d** texture) {
            *texture = component->texture.raw();
        }

        static void SetTexture(sprite_renderer_component* component, texture_2d* texture) {
            component->texture = texture;
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

        static uint16_t GetFilterCategory(rigid_body_component* component) {
            return component->filter_category;
        }

        static void SetFilterCategory(rigid_body_component* component, uint16_t category) {
            component->filter_category = category;
        }

        static uint16_t GetFilterMask(rigid_body_component* component) {
            return component->filter_mask;
        }

        static void SetFilterMask(rigid_body_component* component, uint16_t mask) {
            component->filter_mask = mask;
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

        static void LoadTexture2D(void* path, texture_2d** address) {
            fs::path texture_path = script_engine::from_managed_string(path);
            auto texture = texture_2d::load(texture_path);

            ref_counter counter(texture.raw());
            counter.add();

            *address = texture.raw();
        }

        static texture_wrap GetWrapTexture2D(texture_2d* address) { return address->get_wrap(); }

        static texture_filter GetFilterTexture2D(texture_2d* address) {
            return address->get_filter();
        }

        static bool IsScriptEnabled(script_component* component) { return component->enabled; }

        static void SetScriptEnabled(script_component* component, bool enabled) {
            if (component->gc_handle == 0) {
                return;
            }

            component->enabled = enabled;
        }

        static void* GetScript(script_component* component, void* _entity) {
            entity e = script_helpers::get_entity_from_object(_entity);
            component->verify_script(e);

            if (component->gc_handle == 0) {
                return nullptr;
            }

            return garbage_collector::get_ref_data(component->gc_handle);
        }

        static void* GetChangedFilePath(file_changed_event* e) {
            const auto& path = e->get_path();
            return script_engine::to_managed_string(path.string());
        }

        static void* GetWatchedDirectory(file_changed_event* e) {
            const auto& directory = e->get_watched_directory();
            return script_engine::to_managed_string(directory.string());
        }

        static void GetFileStatus(file_changed_event* e, file_status* status) {
            *status = e->get_status();
        }

        static void* GetAssetPath(asset* a) {
            const auto& path = a->get_path();
            return script_engine::to_managed_string(path.string());
        }

        static void GetAssetType(asset* a, asset_type* type) { *type = a->get_asset_type(); }
        static guid GetAssetGUID(asset* a) { return a->id; }
    } // namespace internal_script_calls

    template <typename T>
    static void add_ref(T* address) {
        ref_counter<T> counter(address);
        counter.add();
    }

    template <typename T>
    static void remove_ref(T* address) {
        ref_counter<T> counter(address);
        counter.remove();
    }

    void script_engine::register_internal_script_calls() {
        register_component_types();

#define REGISTER_CALL(managed, native)                                                             \
    register_internal_call("SGE.InternalCalls::" managed, (void*)native)

#define REGISTER_FUNC(name) REGISTER_CALL(#name, internal_script_calls::name)
#define REGISTER_REF_COUNTER(type)                                                                 \
    REGISTER_CALL("AddRef_" #type, add_ref<type>);                                                 \
    REGISTER_CALL("RemoveRef_" #type, remove_ref<type>)

        // scene
        REGISTER_FUNC(CreateEntity);
        REGISTER_FUNC(CreateEntityWithGUID);
        REGISTER_FUNC(CloneEntity);
        REGISTER_FUNC(DestroyEntity);
        REGISTER_FUNC(FindEntity);
        REGISTER_FUNC(GetCollisionCategoryName);

        // entity
        REGISTER_FUNC(AddComponent);
        REGISTER_FUNC(HasComponent);
        REGISTER_FUNC(GetComponent);
        REGISTER_FUNC(GetGUID);

        // guid
        REGISTER_FUNC(GenerateGUID);

        // tag component
        REGISTER_FUNC(SetTag);
        REGISTER_FUNC(GetTag);

        // transform component
        REGISTER_FUNC(GetTranslation);
        REGISTER_FUNC(SetTranslation);
        REGISTER_FUNC(GetRotation);
        REGISTER_FUNC(SetRotation);
        REGISTER_FUNC(GetScale);
        REGISTER_FUNC(SetScale);

        // sprite renderer component
        REGISTER_FUNC(GetColor);
        REGISTER_FUNC(SetColor);
        REGISTER_FUNC(GetTexture);
        REGISTER_FUNC(SetTexture);

        // camera component
        REGISTER_FUNC(GetPrimary);
        REGISTER_FUNC(SetPrimary);
        REGISTER_FUNC(GetProjectionType);
        REGISTER_FUNC(SetProjectionType);
        REGISTER_FUNC(GetViewSize);
        REGISTER_FUNC(SetViewSize);
        REGISTER_FUNC(GetFOV);
        REGISTER_FUNC(SetFOV);
        REGISTER_FUNC(GetOrthographicClips);
        REGISTER_FUNC(SetOrthographicClips);
        REGISTER_FUNC(GetPerspectiveClips);
        REGISTER_FUNC(SetPerspectiveClips);
        REGISTER_FUNC(SetOrthographic);
        REGISTER_FUNC(SetPerspective);

        // rigid body component
        REGISTER_FUNC(GetBodyType);
        REGISTER_FUNC(SetBodyType);
        REGISTER_FUNC(GetFixedRotation);
        REGISTER_FUNC(SetFixedRotation);
        REGISTER_FUNC(GetAngularVelocity);
        REGISTER_FUNC(SetAngularVelocity);
        REGISTER_FUNC(AddForce);
        REGISTER_FUNC(GetFilterCategory);
        REGISTER_FUNC(SetFilterCategory);
        REGISTER_FUNC(GetFilterMask);
        REGISTER_FUNC(SetFilterMask);

        // box collider component
        REGISTER_FUNC(GetSize);
        REGISTER_FUNC(SetSize);
        REGISTER_FUNC(GetDensity);
        REGISTER_FUNC(SetDensity);
        REGISTER_FUNC(GetFriction);
        REGISTER_FUNC(SetFriction);
        REGISTER_FUNC(GetRestitution);
        REGISTER_FUNC(SetRestitution);
        REGISTER_FUNC(GetRestitutionThreashold);
        REGISTER_FUNC(SetRestitutionThreashold);

        // logger
        REGISTER_FUNC(LogDebug);
        REGISTER_FUNC(LogInfo);
        REGISTER_FUNC(LogWarn);
        REGISTER_FUNC(LogError);

        // input
        REGISTER_FUNC(GetKey);
        REGISTER_FUNC(GetMouseButton);
        REGISTER_FUNC(GetMousePosition);

        // event
        REGISTER_FUNC(IsEventHandled);
        REGISTER_FUNC(SetEventHandled);

        // window events
        REGISTER_FUNC(GetResizeWidth);
        REGISTER_FUNC(GetResizeHeight);

        // input events
        REGISTER_FUNC(GetPressedEventKey);
        REGISTER_FUNC(GetRepeatCount);
        REGISTER_FUNC(GetReleasedEventKey);
        REGISTER_FUNC(GetTypedEventKey);
        REGISTER_FUNC(GetEventMousePosition);
        REGISTER_FUNC(GetScrollOffset);
        REGISTER_FUNC(GetEventMouseButton);
        REGISTER_FUNC(GetMouseButtonReleased);

        // texture_2d
        REGISTER_REF_COUNTER(texture_2d);
        REGISTER_FUNC(LoadTexture2D);
        REGISTER_FUNC(GetWrapTexture2D);
        REGISTER_FUNC(GetFilterTexture2D);

        // script component
        REGISTER_FUNC(IsScriptEnabled);
        REGISTER_FUNC(SetScriptEnabled);
        REGISTER_FUNC(GetScript);

        // file changed event
        REGISTER_FUNC(GetChangedFilePath);
        REGISTER_FUNC(GetWatchedDirectory);
        REGISTER_FUNC(GetFileStatus);

        // assets
        REGISTER_FUNC(GetAssetPath);
        REGISTER_FUNC(GetAssetType);
        REGISTER_FUNC(GetAssetGUID);

#undef REGISTER_CALL
#undef REGISTER_FUNC
#undef REGISTER_REF_COUNTER
    }
} // namespace sge
