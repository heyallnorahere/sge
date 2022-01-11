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
#include "sge/renderer/uniform_buffer.h"
#include "sge/platform/vulkan/vulkan_buffer.h"
namespace sge {
    class vulkan_uniform_buffer : public uniform_buffer {
    public:
        vulkan_uniform_buffer(size_t size);
        virtual ~vulkan_uniform_buffer() override = default;

        virtual size_t get_size() override { return m_buffer->size(); }

        virtual void set_data(const void* data, size_t size, size_t offset) override;

        ref<vulkan_buffer> get() { return m_buffer; }
        const VkDescriptorBufferInfo& get_descriptor_info() { return m_descriptor_info; }

    private:
        ref<vulkan_buffer> m_buffer;
        VkDescriptorBufferInfo m_descriptor_info;
    };
} // namespace sge