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

#pragma once
namespace sge {
    class vertex_buffer : public ref_counted {
    public:
        template <typename T> static ref<vertex_buffer> create(const std::vector<T>& data) {
            return create(data.data(), sizeof(T), data.size());
        }
        static ref<vertex_buffer> create(const void* data, size_t stride, size_t count);

        virtual ~vertex_buffer() = default;

        virtual size_t get_vertex_stride() = 0;
        virtual size_t get_vertex_count() = 0;
        size_t get_total_size() { return this->get_vertex_count() * this->get_vertex_stride(); }
    };
} // namespace sge