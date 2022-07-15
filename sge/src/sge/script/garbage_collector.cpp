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
#include "sge/script/garbage_collector.h"
#include "sge/script/mono_include.h"
#include <mono/metadata/mono-gc.h>
namespace sge {
    struct gc_data_t {
        std::unordered_set<object_ref*> refs;
    };

    static std::unique_ptr<gc_data_t> gc_data;
    bool object_ref::get_all(std::vector<ref<object_ref>>& refs) {
        refs.clear();
        if (!gc_data) {
            return false;
        }

        for (auto ptr : gc_data->refs) {
            refs.push_back(ptr);
        }

        return true;
    }

    ref<object_ref> object_ref::from_object(void* object, bool weak) {
        auto result = ref<object_ref>::create();
        result->set(object, weak);
        return result;
    }

    void object_ref::set(void* object, bool weak) {
        if (m_handle != 0) {
            destroy();
        }

        auto mono_object = (MonoObject*)object;
        if (weak) {
            m_handle = mono_gchandle_new_weakref(mono_object, false);
        } else {
            m_handle = mono_gchandle_new(mono_object, false);
        }

        if (m_handle == 0) {
            throw std::runtime_error("could not create a garbage collector ref!");
        }

        m_weak = weak;
        gc_data->refs.insert(this);
    }

    bool object_ref::destroy() {
        if (m_handle == 0) {
            return false;
        }

        bool destroyed = true;
        if (!m_weak) {
            mono_gchandle_free(m_handle);
        } else if (get() == nullptr) {
            destroyed = false;
        }

        gc_data->refs.erase(this);
        reset();

        return destroyed;
    }

    void* object_ref::get() {
        if (m_handle == 0) {
            return nullptr;
        }

        MonoObject* object = mono_gchandle_get_target(m_handle);
        if (object != nullptr && mono_object_get_vtable(object) == nullptr) {
            return nullptr;
        }

        if (m_weak && object == nullptr) {
            destroy();
        }

        return object;
    }

    void object_ref::reset() {
        m_handle = 0;
        m_weak = false;
    }

    void garbage_collector::init() {
        if (gc_data) {
            throw std::runtime_error("the garbage collector has already been initialized!");
        }

        gc_data = std::make_unique<gc_data_t>();
    }

    void garbage_collector::shutdown() {
        if (!gc_data) {
            throw std::runtime_error("the garbage collector has not been initialized!");
        }

        if (!gc_data->refs.empty()) {
            spdlog::warn("a memory leak has been detected!");
        }

        std::unordered_set<object_ref*> refs(gc_data->refs.begin(), gc_data->refs.end());
        for (auto object : refs) {
            object->destroy();
        }

        collect(true);
        gc_data.reset();
    }

    void garbage_collector::collect(bool wait) {
        mono_gc_collect(mono_gc_max_generation());

        if (wait) {
            while (mono_gc_pending_finalizers()) {
                // nothing
            }
        }
    }
} // namespace sge