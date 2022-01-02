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

#include "sgepch.h"
#include "sge/renderer/renderer.h"
#include "sge/renderer/shader.h"
#include "sge/core/application.h"
#ifdef SGE_USE_VULKAN
#include "sge/platform/vulkan/vulkan_renderer.h"
#endif
namespace sge {
    struct vertex {
        glm::vec2 position;
        glm::vec4 color;
        glm::vec2 uv;
        int32_t texture_index;
    };

    struct quad_t {
        glm::vec2 position, size;
        glm::vec4 color;
    };

    struct batch_t {
        std::string shader_name;
        std::vector<quad_t> quads;
    };

    struct vertex_data_t {
        ref<vertex_buffer> vertices;
        ref<index_buffer> indices;
    };

    struct rendering_scene_t {
        std::unique_ptr<batch_t> current_batch;
        std::vector<vertex_data_t> vertex_data;
    };

    struct shader_dependency_t {
        std::unordered_set<pipeline*> pipelines;

        bool empty() { return this->pipelines.empty(); }
    };

    struct camera_data_t {
        glm::mat4 view_projection;
    };

    static struct {
        std::unique_ptr<shader_library> _shader_library;
        std::unique_ptr<renderer_api> api;
        std::map<command_list_type, ref<command_queue>> queues;

        std::unordered_map<ref<shader>, shader_dependency_t> shader_dependencies;

        std::unique_ptr<rendering_scene_t> current_scene;
        std::unordered_map<std::string, ref<pipeline>> pipelines;
        std::vector<std::vector<vertex_data_t>> frame_vertex_data;
        command_list* cmdlist = nullptr;

        ref<uniform_buffer> camera_buffer;
    } renderer_data;

    static void load_shaders() {
        shader_library& library = *renderer_data._shader_library;

        library.add("default", "assets/shaders/default.hlsl");
    }

    void renderer::init() {
        {
            renderer_api* api_instance = nullptr;

#ifdef SGE_USE_VULKAN
            if (api_instance == nullptr) {
                api_instance = new vulkan_renderer;
            }
#endif

            renderer_data.api = std::unique_ptr<renderer_api>(api_instance);
        }

        renderer_data.api->init();

        renderer_data._shader_library = std::make_unique<shader_library>();
        load_shaders();

        renderer_data.camera_buffer = uniform_buffer::create(sizeof(camera_data_t));
    }

    void renderer::shutdown() {
        renderer_data.camera_buffer.reset();

        renderer_data.frame_vertex_data.clear();
        renderer_data.pipelines.clear();
        renderer_data._shader_library.reset();
        renderer_data.queues.clear();

        renderer_data.api->shutdown();
        renderer_data.api.reset();
    }

    void renderer::new_frame() {
        if (renderer_data.frame_vertex_data.empty()) {
            return;
        }

        swapchain& swap_chain = application::get().get_swapchain();
        size_t current_image = swap_chain.get_current_image_index();
        renderer_data.frame_vertex_data[current_image].clear();
    }

    void renderer::add_shader_dependency(ref<shader> _shader, pipeline* _pipeline) {
        if (renderer_data.shader_dependencies.find(_shader) ==
            renderer_data.shader_dependencies.end()) {
            renderer_data.shader_dependencies.insert(
                std::make_pair(_shader, shader_dependency_t()));
        }

        renderer_data.shader_dependencies[_shader.raw()].pipelines.insert(_pipeline);
    }

    void renderer::remove_shader_dependency(ref<shader> _shader, pipeline* _pipeline) {
        if (renderer_data.shader_dependencies.find(_shader) !=
            renderer_data.shader_dependencies.end()) {
            auto& dependency = renderer_data.shader_dependencies[_shader];
            dependency.pipelines.erase(_pipeline);

            if (dependency.empty()) {
                renderer_data.shader_dependencies.erase(_shader);
            }
        }
    }

    void renderer::on_shader_reloaded(ref<shader> _shader) {
        if (renderer_data.shader_dependencies.find(_shader) ==
            renderer_data.shader_dependencies.end()) {
            return;
        }
        const auto& dependency = renderer_data.shader_dependencies[_shader];

        for (auto _pipeline : dependency.pipelines) {
            _pipeline->invalidate();
        }
    }

    shader_library& renderer::get_shader_library() { return *renderer_data._shader_library; }

    ref<command_queue> renderer::get_queue(command_list_type type) {
        ref<command_queue> queue;

        if (renderer_data.queues.find(type) == renderer_data.queues.end()) {
            queue = command_queue::create(type);
            renderer_data.queues.insert(std::make_pair(type, queue));
        } else {
            queue = renderer_data.queues[type];
        }

        return queue;
    }

    void renderer::begin_scene(const glm::mat4& view_projection) {
        if (renderer_data.current_scene) {
            throw std::runtime_error("a scene is already rendering!");
        }

        camera_data_t camera_data;
        camera_data.view_projection = view_projection;
        renderer_data.camera_buffer->set_data(camera_data);

        renderer_data.current_scene = std::make_unique<rendering_scene_t>();
        begin_batch();
    }

    void renderer::end_scene() {
        if (!renderer_data.current_scene) {
            throw std::runtime_error("there is no scene rendering!");
        }
        auto& scene = renderer_data.current_scene;

        flush_batch();

        auto& app = application::get();
        swapchain& swap_chain = app.get_swapchain();

        if (renderer_data.frame_vertex_data.empty()) {
            renderer_data.frame_vertex_data.resize(swap_chain.get_image_count());
        }

        size_t current_image = swap_chain.get_current_image_index();
        auto& frame_vertex_data = renderer_data.frame_vertex_data[current_image];
        frame_vertex_data.insert(frame_vertex_data.end(), scene->vertex_data.begin(),
                                 scene->vertex_data.end());

        scene.reset();
    }

    void renderer::set_command_list(command_list& cmdlist) { renderer_data.cmdlist = &cmdlist; }

    void renderer::begin_batch(const std::string& shader_name) {
        auto& scene = *renderer_data.current_scene;
        scene.current_batch = std::make_unique<batch_t>();
        scene.current_batch->shader_name = shader_name;
    }

    void renderer::next_batch(const std::string& shader_name) {
        flush_batch();
        begin_batch(shader_name);
    }

    void renderer::flush_batch() {
        auto& scene = *renderer_data.current_scene;
        auto& batch = scene.current_batch;
        if (!batch) {
            return;
        }

        if (!batch->quads.empty()) {
            if (renderer_data.cmdlist == nullptr) {
                throw std::runtime_error("cannot add commands to an empty command list!");
            }

            std::vector<vertex> vertices;
            std::vector<uint32_t> indices;
            for (const auto& quad : batch->quads) {
                std::vector<uint32_t> quad_indices = { 0, 1, 3, 1, 2, 3 };
                for (uint32_t& index : quad_indices) {
                    index += (uint32_t)vertices.size();
                }
                indices.insert(indices.end(), quad_indices.begin(), quad_indices.end());

                // until textures
                static constexpr int32_t texture_index = 0;

                std::vector<vertex> quad_vertices(4);

                // top right
                vertex* v = &quad_vertices[0];
                v->position = quad.position + quad.size;
                v->color = quad.color;
                v->uv = glm::vec2(1.f, 0.f);
                v->texture_index = texture_index;

                // bottom right
                v = &quad_vertices[1];
                v->position = quad.position + glm::vec2(quad.size.x, 0.f);
                v->color = quad.color;
                v->uv = glm::vec2(1.f, 1.f);
                v->texture_index = texture_index;

                // bottom left
                v = &quad_vertices[2];
                v->position = quad.position;
                v->color = quad.color;
                v->uv = glm::vec2(0.f, 1.f);
                v->texture_index = texture_index;

                // top left
                v = &quad_vertices[3];
                v->position = quad.position + glm::vec2(0.f, quad.size.y);
                v->color = quad.color;
                v->uv = glm::vec2(0.f, 0.f);
                v->texture_index = texture_index;

                vertices.insert(vertices.end(), quad_vertices.begin(), quad_vertices.end());
            }

            swapchain& swap_chain = application::get().get_swapchain();
            auto renderpass = swap_chain.get_render_pass();

            ref<pipeline> _pipeline;
            if (renderer_data.pipelines.find(batch->shader_name) != renderer_data.pipelines.end()) {
                _pipeline = renderer_data.pipelines[batch->shader_name];
            } else {
                ref<shader> _shader = renderer_data._shader_library->get(batch->shader_name);
                if (!_shader) {
                    throw std::runtime_error("shader " + batch->shader_name + " does not exist!");
                }

                pipeline_spec spec;
                spec._shader = _shader;
                spec.renderpass = renderpass;
                spec.input_layout.stride = sizeof(vertex);
                spec.input_layout.attributes = {
                    { vertex_attribute_type::float2, offsetof(vertex, position) },
                    { vertex_attribute_type::float4, offsetof(vertex, color) },
                    { vertex_attribute_type::float2, offsetof(vertex, uv) },
                    { vertex_attribute_type::int1, offsetof(vertex, texture_index) }
                };

                _pipeline = pipeline::create(spec);
                _pipeline->set_uniform_buffer(renderer_data.camera_buffer, 0);
                renderer_data.pipelines.insert(std::make_pair(batch->shader_name, _pipeline));
            }

            draw_data data;
            data.cmdlist = renderer_data.cmdlist;
            data.vertices = vertex_buffer::create(vertices);
            data.indices = index_buffer::create(indices);
            data._pipeline = _pipeline;
            renderer_data.api->submit(data);

            vertex_data_t vertex_data;
            vertex_data.vertices = data.vertices;
            vertex_data.indices = data.indices;
            scene.vertex_data.push_back(vertex_data);
        }

        batch.reset();
    }

    void renderer::draw_quad(glm::vec2 position, glm::vec2 size, glm::vec4 color) {
        auto& batch = *renderer_data.current_scene->current_batch;

        quad_t quad;
        quad.position = position;
        quad.size = size;
        quad.color = color;

        batch.quads.push_back(quad);
    }
} // namespace sge