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
            void* constructor = script_engine::get_method(_class, ".ctor(IntPtr)");
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
        static bool AreRefsEqual(void* lhs, void* rhs) {
            void* ptr1 = *(void**)lhs;
            void* ptr2 = *(void**)rhs;
            return ptr1 == ptr2;
        }

        static void GetRefPointer(void* nativeRef, void** pointer) {
            *pointer = *(void**)nativeRef;
        }

        static void DestroySceneRef(void* nativeRef) { delete (ref<scene>*)nativeRef; }

        static bool HasComponent(void* componentType, uint32_t entityID, void* _scene) {
            verify_component_type_validity(componentType);

            auto scene_ref = *(ref<scene>*)_scene;
            entity e((entt::entity)entityID, scene_ref.raw());

            const auto& callbacks = internal_script_call_data.component_callbacks[componentType];
            return callbacks.has(e);
        }

        static void* GetComponent(void* componentType, uint32_t entityID, void* _scene) {
            verify_component_type_validity(componentType);

            auto scene_ref = *(ref<scene>*)_scene;
            entity e((entt::entity)entityID, scene_ref.raw());

            const auto& callbacks = internal_script_call_data.component_callbacks[componentType];
            return callbacks.get(e);
        }

        static guid GetGUID(uint32_t entityID, void* _scene) {
            auto scene_ref = *(ref<scene>*)_scene;
            entity e((entt::entity)entityID, scene_ref.raw());
            return e.get_guid();
        }

        static guid GenerateGUID() { return guid(); }
    } // namespace internal_script_calls

    void register_internal_script_calls() {
        register_component_types();

#define REGISTER_CALL(name)                                                                        \
    script_engine::register_internal_call("SGE.InternalCalls::" #name,                             \
                                          (void*)internal_script_calls::name)

        // ref
        REGISTER_CALL(AreRefsEqual);
        REGISTER_CALL(GetRefPointer);

        // scene
        REGISTER_CALL(DestroySceneRef);

        // entity
        REGISTER_CALL(HasComponent);
        REGISTER_CALL(GetComponent);
        REGISTER_CALL(GetGUID);

        // guid
        REGISTER_CALL(GenerateGUID);

#undef REGISTER_CALL
    }
} // namespace sge