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
#include "sge/renderer/command_queue.h"
#include "sge/renderer/shader.h"
#include "sge/renderer/pipeline.h"
#include "sge/renderer/command_list.h"
#include "sge/renderer/vertex_buffer.h"
#include "sge/renderer/index_buffer.h"
#include "sge/renderer/texture.h"
#include "sge/renderer/render_pass.h"
#include "sge/scene/editor_camera.h"
namespace sge {
    struct draw_data {
        command_list* cmdlist;
        ref<vertex_buffer> vertices;
        ref<index_buffer> indices;
        ref<pipeline> _pipeline;
    };
    
    struct device_info {
        std::string name;
        std::string graphics_api;
    };

    class renderer_api {
    public:
        virtual ~renderer_api() = default;

        virtual void init() = 0;
        virtual void shutdown() = 0;
        virtual void wait() = 0;

        virtual void submit(const draw_data& data) = 0;

        virtual device_info query_device_info() = 0;
    };

    class renderer {
    public:
        renderer() = delete;

        static void init();
        static void shutdown();
        static void new_frame();
        static void wait();

        static void clear_render_data();

        static void add_shader_dependency(ref<shader> _shader, pipeline* _pipeline);
        static void remove_shader_dependency(ref<shader> _shader, pipeline* _pipeline);
        static void on_shader_reloaded(ref<shader> _shader);

        static ref<texture_2d> get_white_texture();
        static ref<texture_2d> get_black_texture();

        static ref<command_queue> get_queue(command_list_type type);
        static shader_library& get_shader_library();

        static void begin_scene(const glm::mat4& view_projection); // todo(nora): camera
        static void end_scene();

        static void set_command_list(command_list& cmdlist);
        static void set_shader(ref<shader> _shader);

        static void begin_batch();
        static void next_batch();
        static void flush_batch();

        static void push_render_pass(ref<render_pass> renderpass, const glm::vec4& clear_color);
        static ref<render_pass> pop_render_pass();
        static void begin_render_pass();

        static size_t push_texture(ref<texture_2d> texture);

        static void draw_grid(const editor_camera& camera);

        static void draw_quad(glm::vec2 position, glm::vec2 size, glm::vec4 color);
        static void draw_quad(glm::vec2 position, glm::vec2 size, glm::vec4 color,
                              ref<texture_2d> texture);

        static void draw_rotated_quad(glm::vec2 position, float rotation, glm::vec2 size,
                                      glm::vec4 color);
        static void draw_rotated_quad(glm::vec2 position, float rotation, glm::vec2 size,
                                      glm::vec4 color, ref<texture_2d> texture);

        struct stats {
            uint32_t draw_calls;
            uint32_t quad_count;

            uint32_t vertex_count;
            uint32_t index_count;

            void reset() {
                draw_calls = 0;
                quad_count = 0;

                vertex_count = 0;
                index_count = 0;
            }
        };
        static stats get_stats();

        static device_info query_device_info();
    };
} // namespace sge
