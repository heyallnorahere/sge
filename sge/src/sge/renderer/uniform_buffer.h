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
    class uniform_buffer : public ref_counted {
    public:
        static ref<uniform_buffer> create(size_t size);

        virtual ~uniform_buffer() = default;

        virtual size_t get_size() = 0;

        virtual void set_data(const void* data, size_t size, size_t offset = 0) = 0;

        template <typename T> void set_data(const T& data, size_t offset = 0) {
            set_data(&data, sizeof(T), offset);
        }
    };
} // namespace sge