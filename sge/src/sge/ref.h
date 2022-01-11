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
    class ref_counted {
    protected:
        ref_counted() { m_ref_count = 0; }

    private:
        mutable int64_t m_ref_count;
        template <typename T>
        friend class ref;
    };
    template <typename T>
    class ref {
    public:
        ref() { m_instance = nullptr; }
        ref(std::nullptr_t) { m_instance = nullptr; }
        ref(T* instance) {
            static_assert(std::is_base_of<ref_counted, T>::value,
                          "class is not a derived type of ref_counted");

            m_instance = instance;
            inrease_ref_count();
        }

        template <typename U>
        ref(const ref<U>& other) {
            static_assert(std::is_base_of_v<T, U>, "polymorphism or something");

            m_instance = (T*)other.m_instance;
            inrease_ref_count();
        }

        template <typename U>
        ref(ref<U>&& other) {
            static_assert(std::is_base_of_v<T, U>, "polymorphism or something");

            m_instance = (T*)other.m_instance;
            other.m_instance = nullptr;
        }

        ~ref() { decrease_ref_count(); }

        ref(const ref<T>& other) {
            m_instance = other.m_instance;
            inrease_ref_count();
        }

        ref& operator=(std::nullptr_t) {
            decrease_ref_count();
            m_instance = nullptr;
            return *this;
        }

        ref& operator=(const ref<T>& other) {
            other.inrease_ref_count();
            decrease_ref_count();
            m_instance = other.m_instance;
            return *this;
        }

        template <typename U>
        ref& operator=(const ref<U>& other) {
            static_assert(std::is_base_of_v<T, U>, "polymorphism or something");

            other.inrease_ref_count();
            decrease_ref_count();
            m_instance = (T*)(void*)other.m_instance;
            return *this;
        }

        template <typename U>
        ref& operator=(ref<U>&& other) {
            static_assert(std::is_base_of_v<T, U>, "polymorphism or something");

            decrease_ref_count();
            m_instance = (T*)(void*)other.m_instance;
            other.m_instance = nullptr;
            return *this;
        }

        operator bool() const { return m_instance != nullptr; }

        T* operator->() const { return m_instance; }
        T& operator*() const { return *m_instance; }
        T* raw() const { return m_instance; }

        void reset(T* instance = nullptr) {
            decrease_ref_count();
            m_instance = instance;
            if (m_instance) {
                inrease_ref_count();
            }
        }

        template <typename U>
        ref<U> as() const {
            return ref<U>((U*)m_instance);
        }

        template <typename... Args>
        static ref<T> create(Args&&... args) {
            T* instance = new T(std::forward<Args>(args)...);
            return ref<T>(instance);
        }

        bool operator==(const ref<T>& other) const { return m_instance == other.m_instance; }
        bool operator!=(const ref<T>& other) const { return !(*this == other); }

        bool equals_object(const ref<T>& other) {
            if (!m_instance || !other.m_instance) {
                return false;
            }
            return *m_instance == *other.m_instance;
        }

    private:
        void inrease_ref_count() const {
            if (m_instance) {
                m_instance->m_ref_count++;
            }
        }

        void decrease_ref_count() const {
            if (m_instance) {
                m_instance->m_ref_count--;
                if (m_instance->m_ref_count <= 0) {
                    delete m_instance;
                    m_instance = nullptr;
                }
            }
        }

        mutable T* m_instance;
        template <typename U>
        friend class ref;
    };
} // namespace sge

namespace std {
    template <typename T>
    struct hash<::sge::ref<T>> {
        size_t operator()(const ::sge::ref<T>& ref) const {
            hash<T*> hasher;
            return hasher(ref.raw());
        }
    };
} // namespace std