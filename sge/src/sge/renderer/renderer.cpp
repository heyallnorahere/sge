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
        float rotation;
        glm::vec4 color;
        size_t texture_index;
    };

    struct batch_t {
        ref<shader> _shader;
        std::vector<quad_t> quads;
        std::vector<ref<texture_2d>> textures;
    };

    struct vertex_data_t {
        ref<vertex_buffer> vertices;
        ref<index_buffer> indices;
    };

    struct rendering_scene_t {
        std::unique_ptr<batch_t> current_batch;
        std::vector<vertex_data_t> vertex_data;
        std::unordered_map<ref<render_pass>, std::vector<ref<pipeline>>> used_pipelines;
    };

    struct shader_dependency_t {
        std::unordered_set<pipeline*> pipelines;

        bool empty() { return this->pipelines.empty(); }
    };

    struct camera_data_t {
        glm::mat4 view_projection;
    };

    struct used_pipeline_data_t {
        std::vector<ref<pipeline>> currently_using;
        std::queue<ref<pipeline>> used;
    };

    struct render_pass_pipeline_data_t {
        std::unordered_map<ref<shader>, used_pipeline_data_t> data;
    };

    struct frame_renderer_data_t {
        std::unordered_map<ref<render_pass>, render_pass_pipeline_data_t> pipelines;
        std::vector<vertex_data_t> vertex_data;
    };

    struct render_pass_data_t {
        ref<render_pass> pass;
        bool active;
        glm::vec4 clear_color;
    };

    static struct {
        std::unique_ptr<shader_library> _shader_library;
        std::unique_ptr<renderer_api> api;
        std::map<command_list_type, ref<command_queue>> queues;

        std::unordered_map<ref<shader>, shader_dependency_t> shader_dependencies;

        std::unique_ptr<rendering_scene_t> current_scene;
        std::vector<frame_renderer_data_t> frame_renderer_data;
        std::stack<render_pass_data_t> render_passes;
        command_list* cmdlist = nullptr;

        ref<uniform_buffer> camera_buffer;
        ref<texture_2d> white_texture, black_texture;

        renderer::stats stats;
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

        {
            texture_2d_spec spec;
            spec.filter = texture_filter::linear;
            spec.wrap = texture_wrap::repeat;

            std::vector<uint8_t> data = { 0, 0, 0, 255 };
            auto img_data = image_data::create(data.data(), data.size() * sizeof(uint8_t), 1, 1,
                                               image_format::RGBA8_SRGB);
            spec.image = image_2d::create(img_data, image_usage_none);
            renderer_data.black_texture = texture_2d::create(spec);

            data = { 255, 255, 255, 255 };
            img_data = image_data::create(data.data(), data.size() * sizeof(uint8_t), 1, 1,
                                          image_format::RGBA8_SRGB);
            spec.image = image_2d::create(img_data, image_usage_none);
            renderer_data.white_texture = texture_2d::create(spec);
        }
    }

    void renderer::shutdown() {
        renderer_data.black_texture.reset();
        renderer_data.white_texture.reset();
        renderer_data.camera_buffer.reset();

        if (!renderer_data.render_passes.empty()) {
            throw std::runtime_error("not all render passes have been popped!");
        }
        renderer_data.frame_renderer_data.clear();
        renderer_data._shader_library.reset();
        renderer_data.queues.clear();

        renderer_data.api->shutdown();
        renderer_data.api.reset();
    }

    void renderer::new_frame() {
        if (renderer_data.frame_renderer_data.empty()) {
            return;
        }

        swapchain& swap_chain = application::get().get_swapchain();
        size_t current_image = swap_chain.get_current_image_index();
        auto& frame_data = renderer_data.frame_renderer_data[current_image];
        frame_data.vertex_data.clear();

        for (auto& [renderpass, pipelines] : frame_data.pipelines) {
            for (auto& [_shader, data] : pipelines.data) {
                for (const auto& _pipeline : data.currently_using) {
                    data.used.push(_pipeline);
                }
                data.currently_using.clear();
            }
        }

        renderer_data.stats.reset();
    }

    void renderer::wait() { renderer_data.api->wait(); }

    void renderer::clear_render_data() {
        wait();

        renderer_data.frame_renderer_data.clear();
        renderer_data.shader_dependencies.clear();
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

    ref<texture_2d> renderer::get_white_texture() { return renderer_data.white_texture; }
    ref<texture_2d> renderer::get_black_texture() { return renderer_data.black_texture; }

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

    shader_library& renderer::get_shader_library() { return *renderer_data._shader_library; }

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

        if (renderer_data.frame_renderer_data.empty()) {
            renderer_data.frame_renderer_data.resize(swap_chain.get_image_count());
        }

        size_t current_image = swap_chain.get_current_image_index();
        auto& frame_renderer_data = renderer_data.frame_renderer_data[current_image];
        frame_renderer_data.vertex_data.insert(frame_renderer_data.vertex_data.end(),
                                               scene->vertex_data.begin(),
                                               scene->vertex_data.end());

        for (const auto& [pass, pipelines] : scene->used_pipelines) {
            auto& pipeline_data = frame_renderer_data.pipelines[pass];

            for (auto _pipeline : pipelines) {
                auto _shader = _pipeline->get_spec()._shader;
                auto& pipelines = pipeline_data.data[_shader];
                pipelines.currently_using.push_back(_pipeline);
            }
        }

        scene.reset();
    }

    void renderer::set_command_list(command_list& cmdlist) { renderer_data.cmdlist = &cmdlist; }

    void renderer::set_shader(ref<shader> _shader) {
        auto& scene = *renderer_data.current_scene;
        if (scene.current_batch->_shader != _shader) {
            next_batch();

            scene.current_batch->_shader = _shader;
        }
    }

    void renderer::begin_batch() {
        auto& scene = *renderer_data.current_scene;
        scene.current_batch = std::make_unique<batch_t>();
        scene.current_batch->_shader = renderer_data._shader_library->get("default");
    }

    void renderer::next_batch() {
        flush_batch();
        begin_batch();
    }

    static glm::vec2 rotate_vertex(glm::vec2 position, float rotation) {
        glm::vec2 normalized = position * 2.f - 1.f;
        float angle = glm::degrees(glm::atan(normalized.y, normalized.x));

        angle -= rotation;

        glm::vec2 new_position;
        new_position.x = glm::cos(glm::radians(angle));
        new_position.y = glm::sin(glm::radians(angle));

        return (new_position + 1.f) / 2.f;
    }

    void renderer::flush_batch() {
        auto& scene = *renderer_data.current_scene;
        auto& batch = scene.current_batch;
        if (!batch) {
            return;
        }

        begin_render_pass();
        auto pass = renderer_data.render_passes.top().pass;

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

                std::vector<vertex> quad_vertices(4);

                // top right
                vertex* v = &quad_vertices[0];
                v->position =
                    quad.position + quad.size * rotate_vertex(glm::vec2(1.f), quad.rotation);
                v->color = quad.color;
                v->uv = glm::vec2(1.f, 0.f);
                v->texture_index = (int32_t)quad.texture_index;

                // bottom right
                v = &quad_vertices[1];
                v->position =
                    quad.position + quad.size * rotate_vertex(glm::vec2(1.f, 0.f), quad.rotation);
                v->color = quad.color;
                v->uv = glm::vec2(1.f, 1.f);
                v->texture_index = (int32_t)quad.texture_index;

                // bottom left
                v = &quad_vertices[2];
                v->position =
                    quad.position + quad.size * rotate_vertex(glm::vec2(0.f), quad.rotation);
                v->color = quad.color;
                v->uv = glm::vec2(0.f, 1.f);
                v->texture_index = (int32_t)quad.texture_index;

                // top left
                v = &quad_vertices[3];
                v->position =
                    quad.position + quad.size * rotate_vertex(glm::vec2(0.f, 1.f), quad.rotation);
                v->color = quad.color;
                v->uv = glm::vec2(0.f, 0.f);
                v->texture_index = (int32_t)quad.texture_index;

                vertices.insert(vertices.end(), quad_vertices.begin(), quad_vertices.end());
            }

            ref<pipeline> _pipeline;
            if (!renderer_data.frame_renderer_data.empty()) {
                swapchain& swap_chain = application::get().get_swapchain();
                size_t image_index = swap_chain.get_current_image_index();
                auto& frame_data = renderer_data.frame_renderer_data[image_index];

                if (frame_data.pipelines[pass].data.find(batch->_shader) !=
                    frame_data.pipelines[pass].data.end()) {
                    auto& queue = frame_data.pipelines[pass].data[batch->_shader].used;
                    if (!queue.empty()) {
                        _pipeline = queue.front();
                        queue.pop();
                    }
                }
            }
            if (!_pipeline) {
                pipeline_spec spec;
                spec._shader = batch->_shader;
                spec.renderpass = pass;
                spec.input_layout.stride = sizeof(vertex);
                spec.input_layout.attributes = {
                    { vertex_attribute_type::float2, offsetof(vertex, position) },
                    { vertex_attribute_type::float4, offsetof(vertex, color) },
                    { vertex_attribute_type::float2, offsetof(vertex, uv) },
                    { vertex_attribute_type::int1, offsetof(vertex, texture_index) }
                };

                _pipeline = pipeline::create(spec);
                _pipeline->set_uniform_buffer(renderer_data.camera_buffer, 0);
            }

            for (size_t i = 0; i < batch->textures.size(); i++) {
                // gonna have to assume 1
                _pipeline->set_texture(batch->textures[i], 1, i);
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

            if (scene.used_pipelines.find(pass) == scene.used_pipelines.end()) {
                scene.used_pipelines.insert(std::make_pair(pass, std::vector<ref<pipeline>>()));
            }
            scene.used_pipelines[pass].push_back(_pipeline);

            renderer_data.stats.draw_calls++;
            renderer_data.stats.quad_count += batch->quads.size();
            renderer_data.stats.vertex_count += vertices.size();
            renderer_data.stats.index_count += indices.size();
        }

        batch.reset();
    }

    void renderer::push_render_pass(ref<render_pass> renderpass, const glm::vec4& clear_color) {
        if (!renderer_data.render_passes.empty()) {
            auto& front = renderer_data.render_passes.top();
            if (front.active) {
                front.pass->end(*renderer_data.cmdlist);
                front.active = false;
            }
        }

        render_pass_data_t pass_data;
        pass_data.pass = renderpass;
        pass_data.clear_color = clear_color;
        pass_data.active = false;
        renderer_data.render_passes.push(pass_data);
    }

    ref<render_pass> renderer::pop_render_pass() {
        auto pass_data = renderer_data.render_passes.top();
        renderer_data.render_passes.pop();

        if (pass_data.active) {
            pass_data.pass->end(*renderer_data.cmdlist);
        }

        return pass_data.pass;
    }

    void renderer::begin_render_pass() {
        auto& pass_data = renderer_data.render_passes.top();
        if (!pass_data.active) {
            pass_data.pass->begin(*renderer_data.cmdlist, pass_data.clear_color);
            pass_data.active = true;
        }
    }

    size_t renderer::push_texture(ref<texture_2d> texture) {
        auto& batch = *renderer_data.current_scene->current_batch;

        std::optional<size_t> texture_index;
        for (size_t i = 0; i < batch.textures.size(); i++) {
            if (batch.textures[i] == texture) {
                texture_index = i;
            }
        }

        if (texture_index.has_value()) {
            return texture_index.value();
        } else {
            size_t index = batch.textures.size();
            batch.textures.push_back(texture);
            return index;
        }
    }

    void renderer::draw_quad(glm::vec2 position, glm::vec2 size, glm::vec4 color) {
        auto& batch = *renderer_data.current_scene->current_batch;

        quad_t quad;
        quad.position = position;
        quad.size = size;
        quad.rotation = 0.f;
        quad.color = color;
        quad.texture_index = push_texture(renderer_data.white_texture);

        batch.quads.push_back(quad);
    }

    void renderer::draw_quad(glm::vec2 position, glm::vec2 size, glm::vec4 color,
                             ref<texture_2d> texture) {
        auto& batch = *renderer_data.current_scene->current_batch;

        quad_t quad;
        quad.position = position;
        quad.size = size;
        quad.rotation = 0.f;
        quad.color = color;
        quad.texture_index = push_texture(texture);

        batch.quads.push_back(quad);
    }

    void renderer::draw_rotated_quad(glm::vec2 position, float rotation, glm::vec2 size,
                                     glm::vec4 color) {
        auto& batch = *renderer_data.current_scene->current_batch;

        quad_t quad;
        quad.position = position;
        quad.size = size;
        quad.rotation = rotation;
        quad.color = color;
        quad.texture_index = push_texture(renderer_data.white_texture);

        batch.quads.push_back(quad);
    }

    void renderer::draw_rotated_quad(glm::vec2 position, float rotation, glm::vec2 size,
                                     glm::vec4 color, ref<texture_2d> texture) {
        auto& batch = *renderer_data.current_scene->current_batch;

        quad_t quad;
        quad.position = position;
        quad.size = size;
        quad.rotation = rotation;
        quad.color = color;
        quad.texture_index = push_texture(texture);

        batch.quads.push_back(quad);
    }

    renderer::stats renderer::get_stats() { return renderer_data.stats; }
    device_info renderer::query_device_info() { return renderer_data.api->query_device_info(); }
} // namespace sge