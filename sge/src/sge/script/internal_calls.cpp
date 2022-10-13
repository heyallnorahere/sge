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
#include "sge/script/script_engine.h"
#include "sge/script/script_helpers.h"
#include "sge/script/garbage_collector.h"
#include "sge/scene/scene.h"
#include "sge/scene/entity.h"
#include "sge/scene/components.h"
#include "sge/scene/prefab.h"
#include "sge/core/input.h"
#include "sge/renderer/shader.h"
#include "sge/asset/sound.h"

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
#pragma region Application

        static void* GetEngineVersion() {
            std::string version = application::get_engine_version();
            return script_engine::to_managed_string(version);
        }

        static void QuitApplication() {
            auto& app = application::get();
            app.quit();
        }

        static void* GetApplicationTitle() {
            auto& app = application::get();
            const auto& title = app.get_title();

            return script_engine::to_managed_string(title);
        }

        static void GetMainWindow(window** _window) {
            auto& app = application::get();
            *_window = app.get_window().raw();
        }

        static bool IsApplicationEditor() {
            auto& app = application::get();
            return app.is_editor();
        }

        static bool IsSubsystemInitialized(subsystem id) {
            auto& app = application::get();
            return app.is_subsystem_initialized(id);
        }

#pragma endregion
#pragma region Window

        static void CreateWindow(void* title, uint32_t width, uint32_t height, window** _window) {
            std::string window_title = script_engine::from_managed_string(title);
            auto created_window = window::create(window_title, width, height);

            auto ptr = created_window.raw();
            if (ptr != nullptr) {
                ref_counter<window> counter(ptr);
                counter++;
            }

            *_window = ptr;
        }

        static uint32_t GetWindowWidth(window* _window) { return _window->get_width(); }
        static uint32_t GetWindowHeight(window* _window) { return _window->get_height(); }

        static void* WindowFileDialog(window* _window, dialog_mode mode, void* filter_list) {
            void* list_type = script_engine::get_class_from_object(filter_list);
            void* count_property = script_engine::get_property(list_type, "Count");

            void* returned = script_engine::get_property_value(filter_list, count_property);
            int32_t count = script_engine::unbox_object<int32_t>(returned);

            void* item_property = script_engine::get_property(list_type, "Item");
            void* filter_type = script_engine::get_property_type(item_property);

            void* name_property = script_engine::get_property(filter_type, "Name");
            void* filter_property = script_engine::get_property(filter_type, "Filter");

            std::vector<dialog_file_filter> filters;
            for (int32_t i = 0; i < count; i++) {
                void* filter = script_engine::get_property_value(filter_list, item_property, &i);
                auto& filter_desc = filters.emplace_back();

                returned = script_engine::get_property_value(filter, name_property);
                filter_desc.name = script_engine::from_managed_string(returned);

                returned = script_engine::get_property_value(filter, filter_property);
                filter_desc.filter = script_engine::from_managed_string(returned);
            }

            auto path = _window->file_dialog(mode, filters);
            if (path.has_value()) {
                auto path_string = path.value().string();
                return script_engine::to_managed_string(path_string);
            }

            return nullptr;
        }

#pragma endregion
#pragma region Scene

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

        static bool FindEntity(guid id, uint32_t* entityID, scene* _scene) {
            entity found_entity = _scene->find_guid(id);
            if (found_entity) {
                *entityID = (uint32_t)found_entity;
                return true;
            }

            return false;
        }

        static void ForEach(void* callback, scene* _scene) {
            auto gc_ref = object_ref::from_object(callback);
            _scene->for_each([gc_ref](entity e) {
                auto delegate = gc_ref->get();

                auto entity_object = script_helpers::create_entity_object(e);
                script_engine::call_delegate(delegate, entity_object);
            });
        }

        static void* GetCollisionCategoryName(scene* _scene, int32_t index) {
            std::string name = _scene->collision_category_name(index);
            return script_engine::to_managed_string(name);
        }

#pragma endregion
#pragma region Entity

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

        static void GetGUID(uint32_t entityID, scene* _scene, guid* id) {
            entity e((entt::entity)entityID, _scene);
            *id = e.get_guid();
        }

#pragma endregion
#pragma region GUID

        static guid GenerateGUID() { return guid(); }

#pragma endregion
#pragma region Collider

        static float GetDensity(collider_data* data) { return data->density; }

        static void SetDensity(collider_data* data, void* _entity, float density) {
            data->density = density;

            entity e = script_helpers::get_entity_from_object(_entity);
            e.get_scene()->update_physics_data(e);
        }

        static float GetFriction(collider_data* data) { return data->friction; }

        static void SetFriction(collider_data* data, void* _entity, float friction) {
            data->friction = friction;

            entity e = script_helpers::get_entity_from_object(_entity);
            e.get_scene()->update_physics_data(e);
        }

        static float GetRestitution(collider_data* data) { return data->restitution; }

        static void SetRestitution(collider_data* data, void* _entity, float restitution) {
            data->restitution = restitution;

            entity e = script_helpers::get_entity_from_object(_entity);
            e.get_scene()->update_physics_data(e);
        }

        static float GetRestitutionThreshold(collider_data* data) {
            return data->restitution_threshold;
        }

        static void SetRestitutionThreshold(collider_data* data, void* _entity, float threshold) {
            data->restitution_threshold = threshold;

            entity e = script_helpers::get_entity_from_object(_entity);
            e.get_scene()->update_physics_data(e);
        }

        static bool IsColliderSensor(collider_data* data) { return data->sensor; }

        static void SetIsColliderSensor(collider_data* data, void* _entity, bool isSensor) {
            data->sensor = isSensor;

            entity e = script_helpers::get_entity_from_object(_entity);
            e.get_scene()->update_physics_data(e);
        }

#pragma endregion
#pragma region TagComponent

        static void SetTag(tag_component* component, void* tag) {
            component->tag = script_engine::from_managed_string(tag);
        }

        static void* GetTag(tag_component* component) {
            return script_engine::to_managed_string(component->tag);
        }

#pragma endregion
#pragma region TransformComponent

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

        static int32_t GetZLayer(transform_component* component) { return component->z_layer; }

        static void SetZLayer(transform_component* component, void* entity_object,
                              int32_t z_layer) {
            component->z_layer = z_layer;

            entity _entity = script_helpers::get_entity_from_object(entity_object);
            _entity.get_scene()->recalculate_render_order();
        }

#pragma endregion
#pragma region SpriteRendererComponent

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

        static void GetShader(sprite_renderer_component* component, shader** result) {
            *result = component->_shader.raw();
        }

        static void SetShader(sprite_renderer_component* component, void* entity_object,
                              shader* _shader) {
            component->_shader = _shader;

            entity _entity = script_helpers::get_entity_from_object(entity_object);
            _entity.get_scene()->recalculate_render_order();
        }

#pragma endregion
#pragma region CameraComponent

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

        static float GetAspectRatio(camera_component* component) {
            return component->camera.get_aspect_ratio();
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

#pragma endregion
#pragma region RigidBodyComponent

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

        static bool GetVelocity(void* _entity, glm::vec2* velocity) {
            entity e = script_helpers::get_entity_from_object(_entity);

            scene* _scene = e.get_scene();
            auto value = _scene->get_velocity(e);

            if (value.has_value()) {
                *velocity = value.value();
                return true;
            }

            return false;
        }

        static bool SetVelocity(void* _entity, glm::vec2 velocity) {
            entity e = script_helpers::get_entity_from_object(_entity);

            scene* _scene = e.get_scene();
            return _scene->set_velocity(e, velocity);
        }

        static bool GetAngularVelocity(void* _entity, float* velocity) {
            entity e = script_helpers::get_entity_from_object(_entity);

            scene* _scene = e.get_scene();
            auto value = _scene->get_angular_velocity(e);

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

        static bool ApplyForce(void* _entity, glm::vec2 force, glm::vec2 point, bool wake) {
            entity e = script_helpers::get_entity_from_object(_entity);

            scene* _scene = e.get_scene();
            return _scene->apply_force(e, force, point, wake);
        }

        static bool ApplyForceToCenter(void* _entity, glm::vec2 force, bool wake) {
            entity e = script_helpers::get_entity_from_object(_entity);

            scene* _scene = e.get_scene();
            return _scene->apply_force(e, force, wake);
        }

        static bool ApplyLinearImpulse(void* _entity, glm::vec2 impulse, glm::vec2 point,
                                       bool wake) {
            entity e = script_helpers::get_entity_from_object(_entity);

            scene* _scene = e.get_scene();
            return _scene->apply_linear_impulse(e, impulse, point, wake);
        }

        static bool ApplyLinearImpulseToCenter(void* _entity, glm::vec2 impulse, bool wake) {
            entity e = script_helpers::get_entity_from_object(_entity);

            scene* _scene = e.get_scene();
            return _scene->apply_linear_impulse(e, impulse, wake);
        }

        static bool ApplyTorque(void* _entity, float torque, bool wake) {
            entity e = script_helpers::get_entity_from_object(_entity);

            scene* _scene = e.get_scene();
            return _scene->apply_torque(e, torque, wake);
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

#pragma endregion
#pragma region BoxColliderComponent

        static void GetBoxSize(box_collider_component* bc, glm::vec2* size) { *size = bc->size; }

        static void SetBoxSize(box_collider_component* bc, void* _entity, glm::vec2 size) {
            bc->size = size;

            entity e = script_helpers::get_entity_from_object(_entity);
            e.get_scene()->update_physics_data(e);
        }

#pragma endregion
#pragma region CircleColliderComponent

        static float GetCircleRadius(circle_collider_component* cc) { return cc->radius; }

        static void SetCircleRadius(circle_collider_component* cc, void* _entity, float radius) {
            cc->radius = radius;

            entity e = script_helpers::get_entity_from_object(_entity);
            e.get_scene()->update_physics_data(e);
        }

#pragma endregion
#pragma region ScriptComponent

        static bool IsScriptEnabled(script_component* component) { return component->enabled; }

        static void SetScriptEnabled(script_component* component, bool enabled) {
            if (!component->instance || component->instance->get() == nullptr) {
                return;
            }

            component->enabled = enabled;
        }

        static void* GetScript(script_component* component, void* _entity) {
            entity e = script_helpers::get_entity_from_object(_entity);
            component->verify_script(e);

            if (!component->instance) {
                return nullptr;
            }

            return component->instance->get();
        }

        static void SetScript(script_component* component, void* _entity, void* script_type) {
            component->remove_script();

            void* script_class = script_engine::from_reflection_type(script_type);
            entity e = script_helpers::get_entity_from_object(_entity);

            component->_class = script_class;
            component->class_name = script_helpers::get_type_name_safe(script_class);

            component->verify_script(e);
        }

        static void RemoveScript(script_component* component) { component->remove_script(); }

#pragma endregion
#pragma region Log

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

#pragma endregion
#pragma region Input

        static bool GetKey(key_code key) { return input::get_key(key); }
        static bool GetMouseButton(mouse_button button) { return input::get_mouse_button(button); }

        static void GetMousePosition(glm::vec2* position) {
            *position = input::get_mouse_position();
        }

#pragma endregion
#pragma region Event

        static bool IsEventHandled(event* address) { return address->handled; }
        static void SetEventHandled(event* address, bool handled) { address->handled = handled; }

#pragma endregion
#pragma region WindowResizeEvent

        static int32_t GetResizeWidth(window_resize_event* address) {
            return (int32_t)address->get_width();
        }

        static int32_t GetResizeHeight(window_resize_event* address) {
            return (int32_t)address->get_height();
        }

#pragma endregion
#pragma region KeyPressedEvent

        static void GetPressedEventKey(key_pressed_event* address, key_code* key) {
            *key = address->get_key();
        }

        static int32_t GetRepeatCount(key_pressed_event* address) {
            return (int32_t)address->get_repeat_count();
        }

#pragma endregion
#pragma region KeyReleasedEvent

        static void GetReleasedEventKey(key_released_event* address, key_code* key) {
            *key = address->get_key();
        }

#pragma endregion
#pragma region KeyTypedEvent

        static void GetTypedEventKey(key_typed_event* address, key_code* key) {
            *key = address->get_key();
        }

#pragma endregion
#pragma region MouseMovedEvent

        static void GetEventMousePosition(mouse_moved_event* address, glm::vec2* position) {
            *position = address->get_position();
        }

#pragma endregion
#pragma region MouseScrolledEvent

        static void GetScrollOffset(mouse_scrolled_event* address, glm::vec2* position) {
            *position = address->get_offset();
        }

#pragma endregion
#pragma region MouseButtonEvent

        static void GetEventMouseButton(mouse_button_event* address, mouse_button* button) {
            *button = address->get_button();
        }

        static bool GetMouseButtonReleased(mouse_button_event* address) {
            return address->get_released();
        }

#pragma endregion
#pragma region FileChangedEvent

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

#pragma endregion
#pragma region Asset

        static void* GetAssetPath(asset* a) {
            const auto& path = a->get_path();
            return script_engine::to_managed_string(path.string());
        }

        static void GetAssetType(asset* a, asset_type* type) { *type = a->get_asset_type(); }
        static void GetAssetGUID(asset* a, guid* id) { *id = a->id; }

        static bool ReloadAsset(asset* a) { return a->reload(); }

#pragma endregion
#pragma region Shader

        static void LoadShaderAuto(void* path, shader** address) {
            fs::path shader_path = script_engine::from_managed_string(path);
            auto _shader = shader::create(shader_path);

            shader* ptr = _shader.raw();
            if (ptr != nullptr) {
                ref_counter<shader> counter(ptr);
                counter++;
            }

            *address = ptr;
        }

        static void LoadShaderExplicit(void* path, shader_language language, shader** address) {
            fs::path shader_path = script_engine::from_managed_string(path);
            auto _shader = shader::create(shader_path, language);

            shader* ptr = _shader.raw();
            if (ptr != nullptr) {
                ref_counter<shader> counter(ptr);
                counter++;
            }

            *address = ptr;
        }

#pragma endregion
#pragma region Texture2D

        static void LoadTexture2D(void* path, texture_2d** address) {
            fs::path texture_path = script_engine::from_managed_string(path);
            auto texture = texture_2d::load(texture_path);

            ref_counter counter(texture.raw());
            counter++;

            *address = texture.raw();
        }

        static texture_wrap GetWrapTexture2D(texture_2d* address) { return address->get_wrap(); }

        static texture_filter GetFilterTexture2D(texture_2d* address) {
            return address->get_filter();
        }

        static void GetTextureImage(texture_2d* address, image_2d** image) {
            auto image_ref = address->get_image();
            auto ptr = image_ref.raw();

            ref_counter<image_2d> counter(ptr);
            counter++;

            *image = ptr;
        }

#pragma endregion
#pragma region Prefab

        static void CreatePrefab(void* entity_object, prefab** result) {
            entity e = script_helpers::get_entity_from_object(entity_object);
            auto _prefab = prefab::from_entity(e);

            prefab* ptr = _prefab.raw();
            ref_counter<prefab> counter(ptr);
            counter++;

            *result = ptr;
        }

        static void* InstantiatePrefab(prefab* _prefab, scene* _scene) {
            entity e = _prefab->instantiate(_scene);
            return script_helpers::create_entity_object(e);
        }

#pragma endregion
#pragma region Sound

        static float GetSoundDuration(sound* _sound) { return _sound->get_duration(); }

        static void PlaySound(sound* _sound, bool repeat,
                              std::weak_ptr<sound_controller>** controller) {
            auto ptr = sound::play(_sound, repeat);
            *controller = new std::weak_ptr<sound_controller>(ptr);
        }

        static bool StopSound(std::weak_ptr<sound_controller>* controller) {
            return sound::stop(*controller);
        }

        static void DeleteSoundControllerPointer(std::weak_ptr<sound_controller>* controller) {
            delete controller;
        }

#pragma endregion
#pragma region Image2D

        static int32_t GetImageWidth(image_2d* image) { return (int32_t)image->get_width(); }
        static int32_t GetImageHeight(image_2d* image) { return (int32_t)image->get_height(); }

        static int32_t GetImageMipLevelCount(image_2d* image) {
            return (int32_t)image->get_mip_level_count();
        }

        static int32_t GetImageArrayLayerCount(image_2d* image) {
            return (int32_t)image->get_array_layer_count();
        }

        static image_format GetImageFormat(image_2d* image) { return image->get_format(); }
        static uint32_t GetImageUsage(image_2d* image) { return image->get_usage(); }

#pragma endregion
    } // namespace internal_script_calls

    template <typename T>
    static void add_ref(T* address) {
        ref_counter<T> counter(address);
        counter++;
    }

    template <typename T>
    static void remove_ref(T* address) {
        ref_counter<T> counter(address);
        counter--;
    }

    void script_engine::register_internal_script_calls() {
        register_component_types();

#define REGISTER_FUNC(name) registerer(#name, internal_script_calls::name)
#define REGISTER_REF_COUNTER(type)                                                                 \
    registerer("AddRef_" #type, add_ref<type>);                                                    \
    registerer("RemoveRef_" #type, remove_ref<type>)

        register_call_group("core", [](const function_registerer& registerer) {
#pragma region Application
            REGISTER_FUNC(GetEngineVersion);
            REGISTER_FUNC(QuitApplication);
            REGISTER_FUNC(GetApplicationTitle);
            REGISTER_FUNC(GetMainWindow);
            REGISTER_FUNC(IsApplicationEditor);
            REGISTER_FUNC(IsSubsystemInitialized);

#pragma endregion
#pragma region Window

            REGISTER_REF_COUNTER(window);
            REGISTER_FUNC(CreateWindow);
            REGISTER_FUNC(GetWindowWidth);
            REGISTER_FUNC(GetWindowHeight);
            REGISTER_FUNC(WindowFileDialog);

#pragma endregion
#pragma region Scene

            REGISTER_FUNC(CreateEntity);
            REGISTER_FUNC(CreateEntityWithGUID);
            REGISTER_FUNC(CloneEntity);
            REGISTER_FUNC(DestroyEntity);
            REGISTER_FUNC(FindEntity);
            REGISTER_FUNC(ForEach);
            REGISTER_FUNC(GetCollisionCategoryName);

#pragma endregion
#pragma region Entity

            REGISTER_FUNC(AddComponent);
            REGISTER_FUNC(HasComponent);
            REGISTER_FUNC(GetComponent);
            REGISTER_FUNC(GetGUID);

#pragma endregion
#pragma region GUID

            REGISTER_FUNC(GenerateGUID);

#pragma endregion
#pragma region Collider

            REGISTER_FUNC(GetDensity);
            REGISTER_FUNC(SetDensity);
            REGISTER_FUNC(GetFriction);
            REGISTER_FUNC(SetFriction);
            REGISTER_FUNC(GetRestitution);
            REGISTER_FUNC(SetRestitution);
            REGISTER_FUNC(GetRestitutionThreshold);
            REGISTER_FUNC(SetRestitutionThreshold);
            REGISTER_FUNC(IsColliderSensor);
            REGISTER_FUNC(SetIsColliderSensor);

#pragma endregion
#pragma region TagComponent

            REGISTER_FUNC(SetTag);
            REGISTER_FUNC(GetTag);

#pragma endregion
#pragma region TransformComponent

            REGISTER_FUNC(GetTranslation);
            REGISTER_FUNC(SetTranslation);
            REGISTER_FUNC(GetRotation);
            REGISTER_FUNC(SetRotation);
            REGISTER_FUNC(GetScale);
            REGISTER_FUNC(SetScale);
            REGISTER_FUNC(GetZLayer);
            REGISTER_FUNC(SetZLayer);

#pragma endregion
#pragma region SpriteRendererComponent

            REGISTER_FUNC(GetColor);
            REGISTER_FUNC(SetColor);
            REGISTER_FUNC(GetTexture);
            REGISTER_FUNC(SetTexture);
            REGISTER_FUNC(GetShader);
            REGISTER_FUNC(SetShader);

#pragma endregion
#pragma region CameraComponent

            REGISTER_FUNC(GetPrimary);
            REGISTER_FUNC(SetPrimary);
            REGISTER_FUNC(GetProjectionType);
            REGISTER_FUNC(SetProjectionType);
            REGISTER_FUNC(GetAspectRatio);
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

#pragma endregion
#pragma region RigidBodyComponent

            REGISTER_FUNC(GetBodyType);
            REGISTER_FUNC(SetBodyType);
            REGISTER_FUNC(GetFixedRotation);
            REGISTER_FUNC(SetFixedRotation);
            REGISTER_FUNC(GetVelocity);
            REGISTER_FUNC(SetVelocity);
            REGISTER_FUNC(GetAngularVelocity);
            REGISTER_FUNC(SetAngularVelocity);
            REGISTER_FUNC(ApplyForce);
            REGISTER_FUNC(ApplyForceToCenter);
            REGISTER_FUNC(ApplyLinearImpulse);
            REGISTER_FUNC(ApplyLinearImpulseToCenter);
            REGISTER_FUNC(ApplyTorque);
            REGISTER_FUNC(GetFilterCategory);
            REGISTER_FUNC(SetFilterCategory);
            REGISTER_FUNC(GetFilterMask);
            REGISTER_FUNC(SetFilterMask);

#pragma endregion
#pragma region BoxColliderComponent

            REGISTER_FUNC(GetBoxSize);
            REGISTER_FUNC(SetBoxSize);

#pragma endregion
#pragma region CircleColliderComponent

            REGISTER_FUNC(GetCircleRadius);
            REGISTER_FUNC(SetCircleRadius);

#pragma endregion
#pragma region ScriptComponent

            REGISTER_FUNC(IsScriptEnabled);
            REGISTER_FUNC(SetScriptEnabled);
            REGISTER_FUNC(GetScript);
            REGISTER_FUNC(SetScript);
            REGISTER_FUNC(RemoveScript);

#pragma endregion
#pragma region Log

            REGISTER_FUNC(LogDebug);
            REGISTER_FUNC(LogInfo);
            REGISTER_FUNC(LogWarn);
            REGISTER_FUNC(LogError);

#pragma endregion
#pragma region Input

            REGISTER_FUNC(GetKey);
            REGISTER_FUNC(GetMouseButton);
            REGISTER_FUNC(GetMousePosition);

#pragma endregion
#pragma region Event

            REGISTER_FUNC(IsEventHandled);
            REGISTER_FUNC(SetEventHandled);

#pragma endregion
#pragma region WindowResizeEvent

            REGISTER_FUNC(GetResizeWidth);
            REGISTER_FUNC(GetResizeHeight);

#pragma endregion
#pragma region KeyPressedEvent

            REGISTER_FUNC(GetPressedEventKey);
            REGISTER_FUNC(GetRepeatCount);

#pragma endregion
#pragma region KeyReleasedEvent

            REGISTER_FUNC(GetReleasedEventKey);

#pragma endregion
#pragma region KeyTypedEvent

            REGISTER_FUNC(GetTypedEventKey);

#pragma endregion
#pragma region MouseMovedEvent

            REGISTER_FUNC(GetEventMousePosition);

#pragma endregion
#pragma region MouseScrolledEvent

            REGISTER_FUNC(GetScrollOffset);

#pragma endregion
#pragma region MouseButtonEvent

            REGISTER_FUNC(GetEventMouseButton);
            REGISTER_FUNC(GetMouseButtonReleased);

#pragma endregion
#pragma region FileChangedEvent

            REGISTER_FUNC(GetChangedFilePath);
            REGISTER_FUNC(GetWatchedDirectory);
            REGISTER_FUNC(GetFileStatus);

#pragma endregion
#pragma region Asset

            REGISTER_FUNC(GetAssetPath);
            REGISTER_FUNC(GetAssetType);
            REGISTER_FUNC(GetAssetGUID);
            REGISTER_FUNC(ReloadAsset);

#pragma endregion
#pragma region Shader

            REGISTER_REF_COUNTER(shader);
            REGISTER_FUNC(LoadShaderAuto);
            REGISTER_FUNC(LoadShaderExplicit);

#pragma endregion
#pragma region Texture2D

            REGISTER_REF_COUNTER(texture_2d);
            REGISTER_FUNC(LoadTexture2D);
            REGISTER_FUNC(GetWrapTexture2D);
            REGISTER_FUNC(GetFilterTexture2D);
            REGISTER_FUNC(GetTextureImage);

#pragma endregion
#pragma region Prefab

            REGISTER_REF_COUNTER(prefab);
            REGISTER_FUNC(CreatePrefab);
            REGISTER_FUNC(InstantiatePrefab);

#pragma endregion
#pragma region Sound

            REGISTER_REF_COUNTER(sound);
            REGISTER_FUNC(GetSoundDuration);
            REGISTER_FUNC(PlaySound);
            REGISTER_FUNC(StopSound);
            REGISTER_FUNC(DeleteSoundControllerPointer);

#pragma endregion
#pragma region Image2D

            REGISTER_REF_COUNTER(image_2d);
            REGISTER_FUNC(GetImageWidth);
            REGISTER_FUNC(GetImageHeight);
            REGISTER_FUNC(GetImageMipLevelCount);
            REGISTER_FUNC(GetImageArrayLayerCount);
            REGISTER_FUNC(GetImageFormat);
            REGISTER_FUNC(GetImageUsage);

#pragma endregion
        });
#undef REGISTER_FUNC
#undef REGISTER_REF_COUNTER
    }
} // namespace sge
