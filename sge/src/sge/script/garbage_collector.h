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
namespace sge {
    class object_ref : public ref_counted {
    public:
        static bool get_all(std::vector<ref<object_ref>>& refs);
        static ref<object_ref> from_object(void* object);

        object_ref() { reset(); }
        ~object_ref() { destroy(); }

        object_ref(const object_ref&) = delete;
        object_ref& operator=(const object_ref&) = delete;

        void set(void* object);
        bool destroy();

        void* get();

    private:
        void reset();

        uint32_t m_handle;
    };

    struct scope_ref {
    public:
        scope_ref(ref<object_ref> object) {
            m_ref = object;
            if (!m_ref || m_ref->get() == nullptr) {
                throw std::runtime_error("invalid ref!");
            }
        }

        ~scope_ref() { m_ref->destroy(); }

        scope_ref(const scope_ref&) = delete;
        scope_ref& operator=(const scope_ref&) = delete;

    private:
        ref<object_ref> m_ref;
    };

    class garbage_collector {
    public:
        garbage_collector() = delete;

        static void init();
        static void shutdown();
        static void collect(bool wait = false);
    };
} // namespace sge