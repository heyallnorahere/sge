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
#include "sge/platform/vulkan/vulkan_buffer.h"
#include "sge/renderer/vertex_buffer.h"
namespace sge {
    class vulkan_vertex_buffer : public vertex_buffer {
    public:
        vulkan_vertex_buffer(const void* data, size_t stride, size_t count);
        virtual ~vulkan_vertex_buffer() override = default;

        virtual size_t get_vertex_stride() override { return this->m_stride; }
        virtual size_t get_vertex_count() override { return this->m_count; }

        ref<vulkan_buffer> get() { return this->m_buffer; }

    private:
        size_t m_stride, m_count;
        ref<vulkan_buffer> m_buffer;
    };
}