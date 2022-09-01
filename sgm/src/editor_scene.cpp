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

#include "sgmpch.h"
#include "editor_scene.h"

#include <sge/renderer/renderer.h>
#include <sge/asset/sound.h>
#include <sge/script/garbage_collector.h>

namespace sgm {
    struct scene_data_t {
        ref<framebuffer> _framebuffer;

        ref<scene> _scene, runtime_scene;
        ref<editor_selection> selection;
        editor_camera camera;
    };

    static std::unique_ptr<scene_data_t> s_scene_data;
    void editor_scene::create() {
        static constexpr uint32_t initial_size = 500;

        s_scene_data = std::make_unique<scene_data_t>();
        s_scene_data->_scene = ref<scene>::create();
        s_scene_data->_scene->set_viewport_size(initial_size, initial_size);
        s_scene_data->camera.update_viewport_size(initial_size, initial_size);

        {
            framebuffer_attachment_spec attachment;
            attachment.type = framebuffer_attachment_type::color;
            attachment.format = image_format::RGBA8_SRGB;
            attachment.additional_usage = image_usage_texture;

            framebuffer_spec spec;
            spec.width = spec.height = initial_size;
            spec.clear_on_load = true;
            spec.enable_blending = true;
            spec.blend_mode = framebuffer_blend_mode::src_alpha_one_minus_src_alpha;
            spec.attachments.push_back(attachment);

            s_scene_data->_framebuffer = framebuffer::create(spec);
        }

        script_helpers::set_editor_scene(s_scene_data->_scene);
    }

    void editor_scene::destroy() {
        script_helpers::set_editor_scene(nullptr);
        s_scene_data.reset();
    }

    void editor_scene::on_update(timestep ts) {
        auto pass = s_scene_data->_framebuffer->get_render_pass();
        renderer::push_render_pass(pass, glm::vec4(0.3f, 0.3f, 0.3f, 1.f));

        if (s_scene_data->runtime_scene) {
            s_scene_data->runtime_scene->on_runtime_update(ts);
        } else {
            s_scene_data->camera.on_update(ts);
            s_scene_data->_scene->on_editor_update(ts, s_scene_data->camera);
        }

        if (renderer::pop_render_pass() != pass) {
            throw std::runtime_error("a render pass was pushed but not popped!");
        }
    }

    void editor_scene::on_event(event& e) {
        if (s_scene_data->runtime_scene) {
            s_scene_data->runtime_scene->on_event(e);
        } else {
            s_scene_data->camera.on_event(e);
        }
    }

    void editor_scene::set_viewport_size(uint32_t width, uint32_t height) {
        s_scene_data->_framebuffer->resize(width, height);
        s_scene_data->camera.update_viewport_size(width, height);

        s_scene_data->_scene->set_viewport_size(width, height);
        if (s_scene_data->runtime_scene) {
            s_scene_data->runtime_scene->set_viewport_size(width, height);
        }
    }

    ref<editor_selection>& editor_scene::get_selection() { return s_scene_data->selection; }
    void editor_scene::reset_selection() { s_scene_data->selection.reset(); }

    void editor_scene::enable_input() { s_scene_data->camera.enable_input(); }
    void editor_scene::disable_input() { s_scene_data->camera.disable_input(); }

    void editor_scene::load(const fs::path& path) {
        if (s_scene_data->runtime_scene) {
            stop();
        }

        reset_selection();

        scene_serializer serializer(s_scene_data->_scene);
        serializer.deserialize(path);
    }

    void editor_scene::save(const fs::path& path) {
        scene_serializer serializer(s_scene_data->_scene);
        serializer.serialize(path);
    }

    bool editor_scene::running() { return s_scene_data->runtime_scene; }

    void editor_scene::play() {
        if (s_scene_data->runtime_scene) {
            return;
        }

        reset_selection();

        s_scene_data->runtime_scene = s_scene_data->_scene->copy();
        s_scene_data->runtime_scene->on_start();

        script_helpers::set_editor_scene(s_scene_data->runtime_scene);
    }

    void editor_scene::stop() {
        if (!s_scene_data->runtime_scene) {
            return;
        }

        reset_selection();
        script_helpers::set_editor_scene(s_scene_data->_scene);

        sound::stop_all();
        garbage_collector::collect(false);

        s_scene_data->runtime_scene->on_stop();
        s_scene_data->runtime_scene.reset();
    }

    ref<scene> editor_scene::get_scene() {
        if (s_scene_data->runtime_scene) {
            return s_scene_data->runtime_scene;
        } else {
            return s_scene_data->_scene;
        }
    }

    ref<framebuffer> editor_scene::get_framebuffer() { return s_scene_data->_framebuffer; }
    editor_camera& editor_scene::get_camera() { return s_scene_data->camera; }
} // namespace sgm