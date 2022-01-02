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
#include "sge/renderer/shader.h"
#include "sge/renderer/render_pass.h"
#include "sge/renderer/uniform_buffer.h"
namespace sge {
    enum class vertex_attribute_type {
        float1,
        float2,
        float3,
        float4,
        int1,
        int2,
        int3,
        int4,
        uint1,
        uint2,
        uint3,
        uint4,
        bool1
    };

    struct vertex_attribute {
        vertex_attribute_type type;
        size_t offset;
    };

    struct pipeline_input_layout {
        size_t stride = 0;
        std::vector<vertex_attribute> attributes;
    };

    struct pipeline_spec {
        ref<shader> _shader;
        pipeline_input_layout input_layout;
        ref<render_pass> renderpass;

        bool enable_culling = true;
        bool wireframe = false;
    };

    class pipeline : public ref_counted {
    public:
        static ref<pipeline> create(const pipeline_spec& spec);

        virtual ~pipeline() = default;

        virtual void invalidate() = 0;

        virtual pipeline_spec& get_spec() = 0;
        virtual const pipeline_spec& get_spec() const = 0;

        virtual void set_uniform_buffer(ref<uniform_buffer> ubo, uint32_t binding) = 0;
    };
} // namespace sge