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
#include "sge/core/layer.h"
namespace sge {
    class layer_stack {
    public:
        using container_t = std::vector<std::unique_ptr<layer>>;

        layer_stack() = default;
        ~layer_stack() { clear(); }

        layer_stack(const layer_stack&) = delete;
        layer_stack& operator=(const layer_stack&) = delete;

        void push_layer(layer* _layer);
        void push_overlay(layer* overlay);
        layer* pop_layer(size_t index = std::numeric_limits<size_t>::max());
        bool pop_layer(layer* _layer);
        layer* pop_overlay(size_t index = std::numeric_limits<size_t>::max());
        bool pop_overlay(layer* overlay);
        void clear();

        size_t size() const { return m_layers.size(); }
        size_t layer_count() const { return m_layer_insert_index; }

        container_t::iterator begin() { return m_layers.begin(); }
        container_t::const_iterator begin() const { return m_layers.end(); }

        container_t::iterator end() { return m_layers.end(); }
        container_t::const_iterator end() const { return m_layers.end(); }

        container_t::reverse_iterator rbegin() { return m_layers.rbegin(); }
        container_t::const_reverse_iterator rbegin() const { return m_layers.rbegin(); }

        container_t::reverse_iterator rend() { return m_layers.rend(); }
        container_t::const_reverse_iterator rend() const { return m_layers.rend(); }

    private:
        container_t m_layers;
        size_t m_layer_insert_index = 0;
    };
} // namespace sge