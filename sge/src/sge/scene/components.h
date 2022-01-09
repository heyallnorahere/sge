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

#pragma once
#include "sge/renderer/texture.h"
#include "sge/scene/runtime_camera.h"
#include "sge/scene/entity_script.h"
#include "sge/scene/entity.h"
#include "sge/core/guid.h"
namespace sge {
    struct id_component {
        guid id;

        id_component() = default;
        id_component(const guid& id) : id(id) {}

        id_component(const id_component&) = default;
        id_component& operator=(const id_component&) = default;
    };

    struct tag_component {
        std::string tag;

        tag_component() = default;
        tag_component(const std::string& t) : tag(t) {}

        tag_component(const tag_component&) = default;
        tag_component& operator=(const tag_component&) = default;
    };

    struct transform_component {
        glm::vec2 translation = glm::vec2(0.f, 0.f);
        float rotation = 0.f; // In degrees
        glm::vec2 scale = glm::vec2(1.f, 1.f);

        transform_component() = default;
        transform_component(const glm::vec3& t) : translation(t) {}

        transform_component(const transform_component&) = default;
        transform_component& operator=(const transform_component&) = default;

        glm::mat4 get_transform() const {
            return glm::translate(glm::mat4(1.0f), glm::vec3(translation, 0.f)) *
                   glm::rotate(glm::mat4(1.f), glm::radians(rotation), glm::vec3(0.f, 0.f, 1.f)) *
                   glm::scale(glm::mat4(1.0f), glm::vec3(scale, 1.0f));
        }
    };

    struct sprite_renderer_component {
        sprite_renderer_component() = default;

        sprite_renderer_component(const sprite_renderer_component&) = default;
        sprite_renderer_component& operator=(const sprite_renderer_component&) = default;

        glm::vec4 color = glm::vec4(1.f);

        // todo: when asset system is implemented, change
        fs::path texture_path;
        ref<texture_2d> texture;
    };

    struct camera_component {
        camera_component() = default;

        camera_component(const camera_component&) = default;
        camera_component& operator=(const camera_component&) = default;

        runtime_camera camera;
        bool primary = true;
    };

    struct native_script_component {
        native_script_component() = default;

        // probably shouldnt use these though
        native_script_component(const native_script_component&) = default;
        native_script_component& operator=(const native_script_component&) = default;

        entity_script* script = nullptr;

        void (*instantiate)(native_script_component* nsc, entity parent) = nullptr;
        void (*destroy)(native_script_component* nsc) = nullptr;

        template <typename T>
        void bind() {
            static_assert(!std::is_same_v<entity_script, T>, "why would you do this");
            static_assert(std::is_base_of_v<entity_script, T>,
                          "cannot cast the given type to entity_script");

            if (script != nullptr) {
                destroy(this);
            }

            instantiate = [](native_script_component* nsc, entity parent) {
                nsc->script = (entity_script*)new T;
                nsc->script->m_parent = parent;
                nsc->script->on_attach();
            };

            destroy = [](native_script_component* nsc) {
                nsc->script->on_detach();
                delete nsc->script;
                nsc->script = nullptr;

                nsc->instantiate = nullptr;
                nsc->destroy = nullptr;
            };
        }
    };

    struct rigid_body_component {
        enum class body_type { static_ = 0, kinematic, dynamic };
        body_type type = body_type::static_;
        bool fixed_rotation = false;

        // Pointer to partner Box2D body object
        void* runtime_body = nullptr;

        rigid_body_component() = default;
        rigid_body_component(const rigid_body_component&) = default;
        rigid_body_component& operator=(const rigid_body_component&) = default;
    };

    struct box_collider_component {
        // This is the size from the origin in all directions.  This defines a unit box centered on
        // the origin (relative to the coordinate space of the entity.)  The size/transformation of
        // the entity is applied to this size. As such, this is the right default values for a box
        // that is the same size as the sprite that is drawn.
        glm::vec2 size = { 0.5f, 0.5f };

        float density = 1.f;
        float friction = 0.5f;
        float restitution = 0.f;
        float restitution_threashold = 0.5f;

        // Pointer to the partner Box2D Fixture object
        void* runtime_fixture = nullptr;

        box_collider_component() = default;
        box_collider_component(const box_collider_component&) = default;
        box_collider_component& operator=(const box_collider_component&) = default;
    };

    //=== scene::on_component_added/on_component_removed ====
    template <>
    inline void scene::on_component_added<camera_component>(entity& entity,
                                                            camera_component& component) {
        uint32_t width = m_viewport_width;
        uint32_t height = m_viewport_height;

        component.camera.set_render_target_size(width, height);
    }

    template <>
    inline void scene::on_component_removed<native_script_component>(
        entity& entity, native_script_component& component) {
        if (component.script != nullptr) {
            component.destroy(&component);
        }
    }

} // namespace sge
