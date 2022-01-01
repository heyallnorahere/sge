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
#include "sge/renderer/index_buffer.h"
#include "sge/platform/vulkan/vulkan_buffer.h"
namespace sge {
    class vulkan_index_buffer : public index_buffer {
    public:
        vulkan_index_buffer(const uint32_t* data, size_t count);
        virtual ~vulkan_index_buffer() override = default;

        virtual size_t get_index_count() override { return this->m_count; }

        ref<vulkan_buffer> get() { return this->m_buffer; }

    private:
        size_t m_count;
        ref<vulkan_buffer> m_buffer;
    };
} // namespace sge