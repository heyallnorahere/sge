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
    class value_wrapper {
    public:
        value_wrapper() {
            m_size = 0;
            m_buffer = nullptr;
        }

        value_wrapper(const void* data, size_t size) {
            m_size = size;
            alloc();

            memcpy(m_buffer, data, m_size);
        }

        value_wrapper(const value_wrapper& other) {
            m_size = other.m_size;
            alloc();

            memcpy(m_buffer, other.m_buffer, m_size);
        }

        value_wrapper(const std::string& value) : value_wrapper(value.data(), value.size()) {}

        template <typename T>
        value_wrapper(const T& value) : value_wrapper(&value, sizeof(T)) {}

        value_wrapper& operator=(const value_wrapper& other) {
            reset();

            m_size = other.m_size;
            alloc();

            memcpy(m_buffer, other.m_buffer, m_size);
        }

        void reset() {
            if (m_buffer != nullptr) {
                m_size = 0;

                free(m_buffer);
                m_buffer = nullptr;
            }
        }

        bool empty() const { return m_size == 0 || m_buffer == nullptr; }
        operator bool() const { return !empty(); }

        template <typename T>
        T get() const {
            if (empty()) {
                throw std::runtime_error("attempted to retrieve a nonexistent value!");
            }

            if constexpr (std::is_same_v<T, std::string>) {
                return T((char*)m_buffer, m_size);
            } else {
                return *(const T*)m_buffer;
            }
        }

        template <typename T>
        T get_or_default(const T& default_value) const {
            if (empty()) {
                return default_value;
            }

            if constexpr (std::is_same_v<T, std::string>) {
                return T((char*)m_buffer, m_size);
            } else {
                return *(const T*)m_buffer;
            }
        }

        const void* ptr() const { return m_buffer; }
        void* ptr() { return m_buffer; }

        size_t size() const { return m_size; }

        static value_wrapper alloc(size_t size) {
            value_wrapper result;

            result.m_size = size;
            result.alloc();

            return result;
        }

    private:
        void alloc() {
            m_buffer = malloc(m_size);
            if (m_buffer == nullptr) {
                throw std::runtime_error("could not allocate data for value wrapper!");
            }
        }

        size_t m_size;
        void* m_buffer;
    };
} // namespace sge
