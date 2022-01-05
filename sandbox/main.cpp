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

#define SGE_INCLUDE_MAIN
#include <sge.h>
#include <sge/renderer/renderer.h>
#include <sge/renderer/framebuffer.h>
namespace sandbox {
    class sandbox_layer : public sge::layer {
    public:
        sandbox_layer() : layer("Sandbox Layer") {}

        virtual void on_attach() override {
            auto image_data = sge::image_data::load("assets/images/tux.png");
            auto img = sge::image_2d::create(image_data, sge::image_usage_texture);

            sge::texture_2d_spec tex_spec;
            tex_spec.image = img;
            tex_spec.filter = sge::texture_filter::linear;
            tex_spec.wrap = sge::texture_wrap::repeat;
            this->m_tux = sge::texture_2d::create(tex_spec);

            this->m_scene = sge::ref<sge::scene>::create();
            this->m_entity = this->m_scene->create_entity("Penguin");

            auto& transform = this->m_entity.get_component<sge::transform_component>();
            transform.scale = glm::vec2(10.f);

            auto& sprite = this->m_entity.add_component<sge::sprite_renderer_component>();
            sprite.texture = this->m_tux;

            auto window = sge::application::get().get_window();
            uint32_t width = window->get_width();
            uint32_t height = window->get_height();

            this->m_camera = this->m_scene->create_entity("Camera");
            auto& camera_data = this->m_camera.add_component<sge::camera_component>();
            camera_data.camera.set_render_target_size(width, height);

            sge::framebuffer_attachment_spec attachment_spec;
            attachment_spec.additional_usage = sge::image_usage_texture;
            attachment_spec.format = sge::image_format::RGBA8_SRGB;
            attachment_spec.type = sge::framebuffer_attachment_type::color;

            sge::framebuffer_spec fb_spec;
            fb_spec.enable_blending = true;
            fb_spec.blend_mode = sge::framebuffer_blend_mode::src_alpha_one_minus_src_alpha;
            fb_spec.width = width;
            fb_spec.height = height;
            fb_spec.clear_on_load = true;
            fb_spec.attachments.push_back(attachment_spec);

            this->m_framebuffer = sge::framebuffer::create(fb_spec);
            this->invalidate_texture();
        }

        virtual void on_detach() override {
            this->m_scene.reset();
            this->m_tux.reset();
        }

        virtual void on_event(sge::event& event) override {
            sge::event_dispatcher dispatcher(event);

            dispatcher.dispatch<sge::window_resize_event>(
                SGE_BIND_EVENT_FUNC(sandbox_layer::on_resize));

            if (!event.handled) {
                this->m_scene->on_event(event);
            }
        }

        virtual void on_update(sge::timestep ts) override {
            auto& transform = this->m_entity.get_component<sge::transform_component>();
            transform.rotation += (float)ts.count() * 25.f;

            auto& camera_data = this->m_camera.get_component<sge::camera_component>();
            float size = camera_data.camera.get_orthographic_size();
            size += (float)ts.count();
            camera_data.camera.set_orthographic_size(size);

            sge::renderer::push_render_pass(this->m_framebuffer->get_render_pass(), glm::vec4(0.f));
            this->m_scene->on_update(ts);
            sge::renderer::pop_render_pass();

            sge::renderer::begin_scene(glm::mat4(1.f));
            sge::renderer::draw_quad(glm::vec2(-1.f), glm::vec2(2.f), glm::vec4(1.f),
                                     this->m_framebuffer_texture);
            sge::renderer::end_scene();

            this->m_time_record += std::chrono::duration_cast<seconds_t>(ts);
            if (this->m_time_record.count() > 1) {
                this->m_time_record -= seconds_t(1);
                spdlog::info("FPS: {0}", 1 / ts.count());
            }
        }

    private:
        void invalidate_texture() {
            sge::texture_2d_spec spec;
            spec.filter = sge::texture_filter::linear;
            spec.wrap = sge::texture_wrap::repeat;
            spec.image =
                this->m_framebuffer->get_attachment(sge::framebuffer_attachment_type::color, 0);
            this->m_framebuffer_texture = sge::texture_2d::create(spec);
        }

        bool on_resize(sge::window_resize_event& event) {
            uint32_t width = event.get_width();
            uint32_t height = event.get_height();

            if (width == 0 || height == 0) {
                spdlog::info("window was minimized");
            } else {
                spdlog::info("window was resized to: ({0}, {1})", width, height);

                this->m_framebuffer->resize(width, height);
                this->invalidate_texture();
            }

            return false;
        }

        using seconds_t = std::chrono::duration<double>;
        seconds_t m_time_record = seconds_t(0);
        sge::ref<sge::scene> m_scene;
        sge::ref<sge::texture_2d> m_tux;
        sge::entity m_entity, m_camera;
        sge::ref<sge::framebuffer> m_framebuffer;
        sge::ref<sge::texture_2d> m_framebuffer_texture;
    };

    class sandbox_app : public sge::application {
    public:
        sandbox_app() : application("Sandbox") {}

    protected:
        virtual void init_app() override {
            this->m_layer = new sandbox_layer;
            this->push_layer(this->m_layer);
        }

        virtual void shutdown_app() override {
            this->pop_layer(this->m_layer);
            delete this->m_layer;
        }

        sandbox_layer* m_layer;
    };
} // namespace sandbox

sge::application* create_app_instance() { return new sandbox::sandbox_app; };