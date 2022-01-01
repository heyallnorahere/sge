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
    class index_buffer : public ref_counted {
    public:
        static ref<index_buffer> create(const uint32_t* data, size_t count);
        static ref<index_buffer> create(const std::vector<uint32_t>& data) {
            return create(data.data(), data.size());
        }

        virtual ~index_buffer() = default;

        virtual size_t get_index_count() = 0;
    };
} // namespace sge