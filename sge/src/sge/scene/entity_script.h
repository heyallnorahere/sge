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
#include "sge/scene/entity.h"
#include "sge/events/event.h"
namespace sge {
    struct native_script_component;
    class entity_script {
    public:
        virtual ~entity_script() = default;

        virtual void on_attach() {}
        virtual void on_detach() {}

        virtual void on_update(timestep ts) {}
        virtual void on_event(event& e) {}

        virtual void on_collision(entity other) {}

    protected:
        template <typename T, typename... Args>
        T& add_component(Args&&... args) {
            return m_parent.add_component<T>(std::forward<Args>(args)...);
        }

        template <typename T>
        T& get_component() {
            return m_parent.get_component<T>();
        }

        template <typename... T>
        bool has_all() {
            return m_parent.has_all<T...>();
        }

        template <typename... T>
        bool has_any() {
            return m_parent.has_any<T...>();
        }

        template <typename T>
        void remove_component() {
            m_parent.remove_component<T>();
        }

        entity m_parent;

        friend struct native_script_component;
    };
} // namespace sge