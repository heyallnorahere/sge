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
    class garbage_collector {
    public:
        garbage_collector() = delete;

        static void init();
        static void shutdown();
        static void collect(bool wait = false);

        static uint32_t create_ref(void* object, bool weak = false);
        static void destroy_ref(uint32_t gc_handle);
        static void* get_ref_data(uint32_t gc_handle);
        static void get_strong_refs(std::vector<uint32_t>& handles);
    };
} // namespace sge