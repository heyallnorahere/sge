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
        T& add_component(Args&&... args) const {
            if (has_all<T>()) {
                throw std::runtime_error("entity already has component!");
            }

            T& component = m_scene->m_registry.emplace<T>(m_handle, std::forward<Args>(args)...);
            m_scene->on_component_added(*this, component);
            return component;
        }

        template <typename T>
        T& get_component() const {
            if (!has_all<T>()) {
                throw std::runtime_error("entity does not have component!");
            }

            return m_scene->m_registry.get<T>(m_handle);
        }

        template <typename T, typename... Args>
        T& ensure_component(Args&&... args) const {
            if (has_all<T>()) {
                return get_component<T>();
            }

            T& component = m_scene->m_registry.emplace<T>(m_handle, std::forward<Args>(args)...);
            m_scene->on_component_added(*this, component);
            return component;
        }

        template <typename T, typename... Args>
        T& add_or_replace_component(Args&&... args) const {
            std::optional<T> original;
            if (has_all<T>()) {
                original = get_component<T>();
            }

            T& component =
                m_scene->m_registry.emplace_or_replace<T>(m_handle, std::forward<Args>(args)...);

            if (original.has_value()) {
                m_scene->on_component_removed(*this, original.value());
            }

            m_scene->on_component_added(*this, component);
            return component;
        }

        template <typename... T>
        bool has_all() const {
            return m_scene->m_registry.all_of<T...>(m_handle);
        }

        template <typename... T>
        bool has_any() const {
            return m_scene->m_registry.any_of<T...>(m_handle);
        }

        template <typename T>
        void remove_component() const {
            if (!has_all<T>()) {
                throw std::runtime_error("entity does not have component!");
            }

            T component = get_component<T>();
            m_scene->m_registry.remove<T>(m_handle);
            m_scene->on_component_removed(*this, component);
        }

        guid get_guid() const { return m_scene->get_guid(*this); }
        scene* get_scene() const { return m_scene; }

        operator bool() const { return m_handle != entt::null; }
        operator entt::entity() const { return m_handle; }
        operator uint32_t() const { return (uint32_t)m_handle; }

        bool operator==(const entity& other) const {
            return m_handle == other.m_handle && m_scene == other.m_scene;
        }

    private:
        entt::entity m_handle{ entt::null };
        scene* m_scene = nullptr;
    };
} // namespace sge