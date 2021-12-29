/*
   Copyright 2021 Nora Beda

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
#include "sge/core/layer_stack.h"
namespace sge {
    void layer_stack::push_layer(layer* _layer) {
        this->m_layers.emplace(this->m_layers.begin() + this->m_layer_insert_index, _layer);
        this->m_layer_insert_index++;
        _layer->on_attach();
    }

    void layer_stack::push_overlay(layer* overlay) {
        this->m_layers.emplace_back(overlay);
        overlay->on_attach();
    }

    layer* layer_stack::pop_layer() {
        if (this->m_layer_insert_index == 0) {
            return nullptr;
        }
        this->m_layer_insert_index--;

        auto it = this->m_layers.begin() + this->m_layer_insert_index;
        layer* _layer = it->get();
        _layer->on_detach();

        it->release();
        this->m_layers.erase(it);
        return _layer;
    }

    layer* layer_stack::pop_overlay() {
        if (this->m_layers.size() - this->m_layer_insert_index == 0) {
            return nullptr;
        }

        auto it = this->m_layers.end() - 1;
        layer* overlay = it->get();
        overlay->on_detach();

        it->release();
        this->m_layers.erase(it);
        return overlay;
    }

    void layer_stack::clear() {
        for (auto& _layer : this->m_layers) {
            _layer->on_detach();
        }
        this->m_layers.clear();
    }
}