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

#include "sge/scene/scene.h"

namespace sge {
    class entity {
    public:
        entity() = default;
        entity(entt::entity handle, scene* scene) : m_handle{ handle }, m_scene{ scene } {}
        entity(const entity& other) = default;

        template <typename T, typename... Args>
        T& add_component(Args&&... args) {
            if (has_all<T>()) {
                throw std::runtime_error("entity already has component!");
            }

            T& component = m_scene->m_registry.emplace<T>(m_handle, std::forward<Args>(args)...);
            m_scene->on_component_added(*this, component);
            return component;
        }

        template <typename T>
        T& get_component() {
            if (!has_all<T>()) {
                throw std::runtime_error("entity does not have component!");
            }

            return m_scene->m_registry.get<T>(m_handle);
        }

        template <typename... T>
        bool has_all() {
            return m_scene->m_registry.all_of<T...>(m_handle);
        }

        template <typename... T>
        bool has_any() {
            return m_scene->m_registry.any_of<T...>(m_handle);
        }

        template <typename T>
        void remove_component() {
            if (!has_all<T>()) {
                throw std::runtime_error("entity does not have component!");
            }

            m_scene->on_component_removed(*this, this->get_component<T>());
            m_scene->m_registry.remove<T>(m_handle);
        }

        operator bool() const { return m_handle != entt::null; }
        operator entt::entity() const { return m_handle; }
        operator uint32_t() const { return (uint32_t)m_handle; }

    private:
        entt::entity m_handle{ entt::null };
        scene* m_scene = nullptr;
    };
} // namespace sge