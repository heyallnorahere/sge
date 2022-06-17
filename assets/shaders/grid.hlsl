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
};

struct vs_output {
    float4 position : SV_POSITION;

    [[vk::location(0)]] float2 uv : TEXCOORD0;
};

vs_output main(vs_input input) {
    vs_output output;

    output.position = float4(input.position, 0.f, 1.0f);
    output.uv = input.uv;

    return output;
}

#stage pixel
struct grid_data_t {
    float view_size;
    float aspect_ratio;
    float2 camera_position;
    uint2 viewport_size;
};
ConstantBuffer<grid_data_t> grid_data : register(b0);

Texture2D textures[16] : register(t1);
SamplerState tex_samplers[16] : register(s1);

float2 get_coords(float2 uv) {
    float2 view_coords = (uv - 0.5f) * grid_data.view_size;
    view_coords *= float2(grid_data.aspect_ratio, -1.f);

    return grid_data.camera_position + view_coords;
}

const float LINE_WIDTH = 1.25f;

float4 main([[vk::location(0)]] float2 uv : TEXCOORD0) : SV_TARGET {
    float2 coords = get_coords(uv);
    float2 line_width = float2(LINE_WIDTH * grid_data.view_size) / float2(grid_data.viewport_size.x);

    bool axis_aligned_x = abs(coords.y) < line_width.y;
    bool axis_aligned_y = abs(coords.x) < line_width.x;

    if (axis_aligned_x || axis_aligned_y) {
        if (!axis_aligned_y) {
            return float4(1.f, 0.f, 0.f, 1.f);
        } else if (!axis_aligned_x) {
            return float4(0.f, 1.f, 0.f, 1.f);
        }
    }

    return float4(0.f);
}