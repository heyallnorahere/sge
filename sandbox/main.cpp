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
    class camera_controller : public sge::entity_script {
    public:
        virtual void on_update(sge::timestep ts) {
            if (sge::input::get_mouse_button(sge::mouse_button::left)) {
                glm::vec2 mouse_position = sge::input::get_mouse_position();
                if (!m_last_mouse_position.has_value()) {
                    m_last_mouse_position = mouse_position;
                }

                glm::vec2 offset =
                    (mouse_position - m_last_mouse_position.value()) * glm::vec2(1.f, -1.f);
                m_last_mouse_position = mouse_position;

                auto window = sge::application::get().get_window();
                uint32_t width = window->get_width();
                uint32_t height = window->get_height();

                glm::vec2 window_size = glm::vec2((float)width, (float)height);
                float aspect_ratio = window_size.x / window_size.y;

                auto& camera_data = get_component<sge::camera_component>();
                float camera_view_size = camera_data.camera.get_orthographic_size();
                glm::vec2 view_size = glm::vec2(camera_view_size * aspect_ratio, camera_view_size);

                auto& transform = get_component<sge::transform_component>();
                transform.translation -= offset * view_size / window_size;
            } else {
                m_last_mouse_position.reset();
            }
        }

        virtual void on_event(sge::event& event) {
            sge::event_dispatcher dispatcher(event);

            dispatcher.dispatch<sge::mouse_scrolled_event>(
                SGE_BIND_EVENT_FUNC(camera_controller::on_scroll));
        }

    private:
        bool on_scroll(sge::mouse_scrolled_event& event) {
            auto& camera_data = get_component<sge::camera_component>();
            float view_size = camera_data.camera.get_orthographic_size();
            view_size -= event.get_offset().y;
            camera_data.camera.set_orthographic_size(view_size);

            return true;
        }

        std::optional<glm::vec2> m_last_mouse_position;
    };

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

            auto window = sge::application::get().get_window();
            uint32_t width = window->get_width();
            uint32_t height = window->get_height();

            this->m_scene = sge::ref<sge::scene>::create();
            this->m_scene->set_viewport_size(width, height);
            this->m_entity = this->m_scene->create_entity("Penguin");

            auto& transform = this->m_entity.get_component<sge::transform_component>();
            transform.scale = glm::vec2(5.f);

            auto& sprite = this->m_entity.add_component<sge::sprite_renderer_component>();
            sprite.texture = this->m_tux;

            this->m_camera = this->m_scene->create_entity("Camera");
            this->m_camera.add_component<sge::camera_component>();
            this->m_camera.add_component<sge::native_script_component>().bind<camera_controller>();
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

            this->m_scene->on_update(ts);

            this->m_time_record += std::chrono::duration_cast<seconds_t>(ts);
            if (this->m_time_record.count() > 1) {
                this->m_time_record -= seconds_t(1);
                spdlog::info("FPS: {0}", 1 / ts.count());
            }
        }

    private:
        bool on_resize(sge::window_resize_event& event) {
            uint32_t width = event.get_width();
            uint32_t height = event.get_height();
            this->m_scene->set_viewport_size(width, height);

            if (width == 0 || height == 0) {
                spdlog::info("window was minimized");
            } else {
                spdlog::info("window was resized to: ({0}, {1})", width, height);
            }

            return false;
        }

        using seconds_t = std::chrono::duration<double>;
        seconds_t m_time_record = seconds_t(0);
        sge::ref<sge::scene> m_scene;
        sge::ref<sge::texture_2d> m_tux;
        sge::entity m_entity, m_camera;
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