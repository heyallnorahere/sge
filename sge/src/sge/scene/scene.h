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
#include <entt/entt.hpp>

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

        void on_update(timestep ts);
        void on_event(event& e);

    private:
        bool on_resize(window_resize_event& e);

        template <typename T>
        void on_component_added(entity& entity, T& component) {
            // no behavior
        }

        entt::registry m_registry;

        friend class entity;
    };
} // namespace sge