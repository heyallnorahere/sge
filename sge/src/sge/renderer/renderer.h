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
#include "sge/core/window.h"
#include "sge/renderer/shader.h"
#include "sge/renderer/pipeline.h"
#include "sge/renderer/command_list.h"
#include "sge/renderer/vertex_buffer.h"
#include "sge/renderer/index_buffer.h"
namespace sge {
    struct draw_data {
        command_list* cmdlist;
        ref<vertex_buffer> vertices;
        ref<index_buffer> indices;
        ref<pipeline> _pipeline;
        // todo(nora): textures
    };

    class renderer_api {
    public:
        virtual ~renderer_api() = default;

        virtual void init() = 0;
        virtual void shutdown() = 0;

        virtual void submit(const draw_data& data) = 0;
    };

    class renderer {
    public:
        renderer() = delete;

        static void init();
        static void shutdown();
        static void new_frame();

        static void add_shader_dependency(ref<shader> _shader, pipeline* _pipeline);
        static void remove_shader_dependency(ref<shader> _shader, pipeline* _pipeline);
        static void on_shader_reloaded(ref<shader> _shader);

        static ref<command_queue> get_queue(command_list_type type);
        static shader_library& get_shader_library();

        static void begin_scene(const glm::mat4& view_projection); // todo(nora): camera
        static void end_scene();

        static void set_command_list(command_list& cmdlist);
        static void begin_batch(const std::string& shader_name = "default");
        static void next_batch(const std::string& shader_name = "default");
        static void flush_batch();

        static void draw_quad(glm::vec2 position, glm::vec2 size, glm::vec4 color);
    };
} // namespace sge