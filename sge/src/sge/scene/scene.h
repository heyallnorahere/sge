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

#include "sge/events/event.h"
#include "sge/events/window_events.h"
#include "sge/scene/editor_camera.h"
#include "sge/core/guid.h"
#include <entt/entt.hpp>

class b2World;

namespace sge {

    class entity;

    // A Scene is a set of entities and components.
    class scene : public ref_counted {
    public:
        scene() = default;
        ~scene() = default;
        scene(const scene&) = delete;
        scene& operator=(const scene&) = delete;

        entity create_entity(const std::string& name = std::string());
        void destroy_entity(entity entity);
        void clear();

        void on_start();
        void on_stop();
        void on_runtime_update(timestep ts);
        void on_editor_update(timestep ts, editor_camera& camera);
        void on_event(event& e);

        void set_viewport_size(uint32_t width, uint32_t height);

        void for_each(const std::function<void(entity)>& callback);

        template <typename... T>
        void for_each(const std::function<void(entity)>& callback) {
            auto view = m_registry.view<T...>();
            for (entt::entity id : view) {
                view_iteration(id, callback);
            }
        }

    private:
        template <typename T>
        void on_component_added(entity& entity, T& component) {
            // no behavior
        }

        template <typename T>
        void on_component_removed(entity& entity, T& component) {
            // no behavior
        }

        void view_iteration(entt::entity id, const std::function<void(entity)>& callback);
        void render(const glm::mat4& view_projection);

        guid get_guid(entity e);

        entt::registry m_registry;
        uint32_t m_viewport_width, m_viewport_height;
        b2World* m_physics_world = nullptr;

        friend class entity;
    };
} // namespace sge