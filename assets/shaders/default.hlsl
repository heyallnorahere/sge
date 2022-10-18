#stage vertex
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

struct vs_input {
    [[vk::location(0)]] float2 position : POSITION0;
    [[vk::location(1)]] float4 color : COLOR0;
    [[vk::location(2)]] float2 uv : TEXCOORD0;
    [[vk::location(3)]] int texture_index : TEXTUREINDEX0;
    [[vk::location(4)]] int flags; // todo: add semantic
};

struct vs_output {
    float4 position : SV_POSITION;

    [[vk::location(0)]] float4 color : COLOR0;
    [[vk::location(1)]] float2 uv : TEXCOORD0;
    [[vk::location(2)]] int texture_index : TEXTUREINDEX0; 
    [[vk::location(3)]] int flags; // todo: add semantic
};

struct camera_data_t {
    float4x4 view_projection;
};

ConstantBuffer<camera_data_t> camera_data : register(b0);

vs_output main(vs_input input) {
    vs_output output;

    output.position = mul(camera_data.view_projection, float4(input.position, 0.f, 1.f));
    output.color = input.color;
    output.uv = input.uv;
    output.texture_index = input.texture_index;
    output.flags = input.flags;

    return output;
}

#stage pixel
struct ps_input {
    [[vk::location(0)]] float4 color : COLOR0;
    [[vk::location(1)]] float2 uv : TEXCOORD0;
    [[vk::location(2)]] int texture_index : TEXTUREINDEX0;
    [[vk::location(3)]] int flags; // todo: add semantic
};

Texture2D textures[16] : register(t1);
SamplerState tex_samplers[16] : register(s1);

float4 main(ps_input input) : SV_TARGET {
    if ((input.flags & (1 << 0)) != 0) {
        float2 center = float2(0.5f, 0.5f);
        float dist = length(input.uv - center);

        if (dist > 0.5f) {
            return float4(0.f, 0.f, 0.f, 0.f);
        }
    }

    float4 tex_color = textures[input.texture_index].Sample(tex_samplers[input.texture_index],
        input.uv);

    return tex_color * input.color;
}