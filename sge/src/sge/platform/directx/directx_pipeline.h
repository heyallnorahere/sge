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
#include "sge/renderer/pipeline.h"
namespace sge {
    class directx_pipeline : public pipeline {
    public:
        directx_pipeline(const pipeline_spec& spec) { m_spec = spec; }
        virtual ~directx_pipeline() override = default;

        virtual void invalidate() override {}

        virtual pipeline_spec& get_spec() override { return m_spec; }
        virtual const pipeline_spec& get_spec() const override { return m_spec; }

        virtual void set_uniform_buffer(ref<uniform_buffer> ubo, uint32_t binding) override {}
        virtual void set_texture(ref<texture_2d> tex, uint32_t binding,
                                 uint32_t slot = 0) override {}

    private:
        pipeline_spec m_spec;
    };
} // namespace sge
