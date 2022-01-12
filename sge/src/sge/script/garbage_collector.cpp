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
        std::unordered_map<uint32_t, void*> strong_map, weak_map;
    };
    static std::unique_ptr<gc_data_t> gc_data;

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

        if (!gc_data->strong_map.empty()) {
            spdlog::warn("a memory leak has been detected!");

            for (auto [gc_handle, object] : gc_data->strong_map) {
                mono_gchandle_free(gc_handle);
            }
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

    uint32_t garbage_collector::create_ref(void* object, bool weak) {
        auto mono_object = (MonoObject*)object;
        uint32_t gc_handle;

        if (weak) {
            gc_handle = mono_gchandle_new_weakref(mono_object, false);
            gc_data->weak_map.insert(std::make_pair(gc_handle, object));
        } else {
            gc_handle = mono_gchandle_new(mono_object, false);
            gc_data->strong_map.insert(std::make_pair(gc_handle, object));
        }

        if (gc_handle == 0) {
            throw std::runtime_error("could not create a garbage collector ref!");
        }
        return gc_handle;
    }

    void garbage_collector::destroy_ref(uint32_t gc_handle) {
        if (gc_data->strong_map.find(gc_handle) == gc_data->strong_map.end()) {
            throw std::runtime_error("invalid gc handle!");
        }

        gc_data->strong_map.erase(gc_handle);
        mono_gchandle_free(gc_handle);
    }

    void* garbage_collector::get_ref_data(uint32_t gc_handle) {
        MonoObject* object = mono_gchandle_get_target(gc_handle);

        if (mono_object_get_vtable(object) == nullptr) {
            return nullptr;
        }
        return object;
    }
} // namespace sge