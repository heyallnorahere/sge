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
namespace sgm {
    struct scene_data_t {
        ref<scene> _scene;
        ref<framebuffer> _framebuffer;
    };
    static std::unique_ptr<scene_data_t> scene_data;

    void editor_scene::create() {
        static constexpr uint32_t initial_size = 500;

        scene_data = std::make_unique<scene_data_t>();
        scene_data->_scene = ref<scene>::create();
        scene_data->_scene->set_viewport_size(initial_size, initial_size);

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

            scene_data->_framebuffer = framebuffer::create(spec);
        }
    }

    void editor_scene::destroy() { scene_data.reset(); }

    void editor_scene::on_update(timestep ts) {
        auto pass = scene_data->_framebuffer->get_render_pass();
        renderer::push_render_pass(pass, glm::vec4(0.3f, 0.3f, 0.3f, 1.f));

        // no play button just yet
        scene_data->_scene->on_editor_update(ts);

        if (renderer::pop_render_pass() != pass) {
            throw std::runtime_error("a render pass was pushed but not popped!");
        }
    }

    void editor_scene::on_event(event& e) { scene_data->_scene->on_event(e); }

    void editor_scene::set_viewport_size(uint32_t width, uint32_t height) {
        scene_data->_framebuffer->resize(width, height);
        scene_data->_scene->set_viewport_size(width, height);
    }

    ref<scene> editor_scene::get_scene() { return scene_data->_scene; }
    ref<framebuffer> editor_scene::get_framebuffer() { return scene_data->_framebuffer; }
}