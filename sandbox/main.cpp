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
using namespace sge;
namespace sandbox {
    class camera_controller : public entity_script {
    public:
        virtual void on_update(timestep ts) {
            if (input::get_mouse_button(mouse_button::left)) {
                glm::vec2 mouse_position = input::get_mouse_position();
                if (!m_last_mouse_position.has_value()) {
                    m_last_mouse_position = mouse_position;
                }

                glm::vec2 offset =
                    (mouse_position - m_last_mouse_position.value()) * glm::vec2(1.f, -1.f);
                m_last_mouse_position = mouse_position;

                auto window = application::get().get_window();
                uint32_t width = window->get_width();
                uint32_t height = window->get_height();

                glm::vec2 window_size = glm::vec2((float)width, (float)height);
                float aspect_ratio = window_size.x / window_size.y;

                auto& camera_data = get_component<camera_component>();
                float camera_view_size = camera_data.camera.get_orthographic_size();
                glm::vec2 view_size = glm::vec2(camera_view_size * aspect_ratio, camera_view_size);

                auto& transform = get_component<transform_component>();
                transform.translation -= offset * view_size / window_size;
            } else {
                m_last_mouse_position.reset();
            }
        }

        virtual void on_event(event& e) {
            event_dispatcher dispatcher(e);

            dispatcher.dispatch<mouse_scrolled_event>(
                SGE_BIND_EVENT_FUNC(camera_controller::on_scroll));
        }

    private:
        bool on_scroll(mouse_scrolled_event& e) {
            auto& camera_data = get_component<camera_component>();
            float view_size = camera_data.camera.get_orthographic_size();
            view_size -= e.get_offset().y;
            camera_data.camera.set_orthographic_size(view_size);

            return true;
        }

        std::optional<glm::vec2> m_last_mouse_position;
    };

    class collision_listener_script : public entity_script {
    public:
        virtual void on_collision(entity other) override {
            auto& my_tag = get_component<tag_component>();
            auto& other_tag = other.get_component<tag_component>();
            spdlog::info("{0}: collided with {1}", my_tag.tag, other_tag.tag);
        }
    };

    class sandbox_layer : public layer {
    public:
        sandbox_layer() : layer("Sandbox Layer") {}

        virtual void on_attach() override {
            auto image_data = image_data::load("assets/images/tux.png");
            auto img = image_2d::create(image_data, image_usage_texture);

            texture_spec tex_spec;
            tex_spec.image = img;
            tex_spec.filter = texture_filter::linear;
            tex_spec.wrap = texture_wrap::repeat;
            m_tux = texture_2d::create(tex_spec);

            auto window = application::get().get_window();
            uint32_t width = window->get_width();
            uint32_t height = window->get_height();

            m_scene = ref<scene>::create();
            m_scene->set_viewport_size(width, height);

            // Tux
            {
                entity tux = m_scene->create_entity("Penguin");

                auto& transform = tux.get_component<transform_component>();
                transform.scale = glm::vec2(5.f);
                transform.rotation = 10.f;

                auto& sprite = tux.add_component<sprite_renderer_component>();
                sprite.texture = m_tux;

                auto& nsc = tux.add_component<native_script_component>();
                nsc.bind<collision_listener_script>();

                auto& rb = tux.add_component<rigid_body_component>();
                rb.type = rigid_body_component::body_type::dynamic;

                auto& bc = tux.add_component<box_collider_component>();
                bc.restitution = 0.8f;
            }

            // Ground
            {
                entity ground = m_scene->create_entity("Ground");

                auto& transform = ground.get_component<transform_component>();
                transform.scale = glm::vec2(100.f, 1.f);
                transform.translation = glm::vec2(0.f, -10.f);

                auto& sprite = ground.add_component<sprite_renderer_component>();
                sprite.color = glm::vec4(0.04f, 0.45f, 0.19f, 1.f);

                auto& rb = ground.add_component<rigid_body_component>();
                rb.type = rigid_body_component::body_type::static_;

                ground.add_component<box_collider_component>();

                auto& nsc = ground.add_component<native_script_component>();
                nsc.bind<collision_listener_script>();
            }

            // Camera
            {
                entity camera = m_scene->create_entity("Camera");

                auto& cc = camera.add_component<camera_component>();
                cc.camera.set_orthographic_size(15.f);

                auto& nsc = camera.add_component<native_script_component>();
                nsc.bind<camera_controller>();

                auto& transform = camera.get_component<transform_component>();
                transform.translation = glm::vec2(0.f, -5.f);
            }

            m_scene->on_start();
        }

        virtual void on_detach() override {
            m_scene->on_stop();
            m_scene.reset();
            m_tux.reset();
        }

        virtual void on_event(event& e) override {
            event_dispatcher dispatcher(e);

            dispatcher.dispatch<window_resize_event>(SGE_BIND_EVENT_FUNC(sandbox_layer::on_resize));

            if (!e.handled) {
                m_scene->on_event(e);
            }
        }

        virtual void on_update(timestep ts) override { m_scene->on_runtime_update(ts); }

        virtual void on_imgui_render() override {
            ImGui::Begin("Sandbox");

            ImGui::Text("Hello!");
            //ImGui::Image(m_tux->get_imgui_id(), ImVec2(50.f, 50.f));

            ImGuiIO& io = ImGui::GetIO();
            ImGui::Text("Running at %f FPS", io.Framerate);

            ImGui::End();
        }

    private:
        bool on_resize(window_resize_event& e) {
            uint32_t width = e.get_width();
            uint32_t height = e.get_height();
            m_scene->set_viewport_size(width, height);

            if (width == 0 || height == 0) {
                spdlog::info("window was minimized");
            } else {
                spdlog::info("window was resized to: ({0}, {1})", width, height);
            }

            return false;
        }

        ref<scene> m_scene;
        ref<texture_2d> m_tux;
    };

    class sandbox_app : public application {
    public:
        sandbox_app() : application("Sandbox") {}

    protected:
        virtual void on_init() override {
            m_layer = new sandbox_layer;
            push_layer(m_layer);
        }

        virtual void on_shutdown() override {
            pop_layer(m_layer);
            delete m_layer;
        }

        sandbox_layer* m_layer;
    };
} // namespace sandbox

application* create_app_instance() { return new sandbox::sandbox_app; }