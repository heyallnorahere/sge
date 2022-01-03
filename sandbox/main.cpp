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
#include <sge/renderer/image.h>
#include <sge/scene/scene.h>
#include <sge/scene/components.h>
#include <sge/scene/entity.h>

namespace sandbox {
    void add_quad_to_scene(sge::ref<sge::scene> scene, const std::string& name, glm::vec2 position,
                           glm::vec2 size, glm::vec4 color) {
        auto e = scene->create_entity(name);
        auto& quad = e.add_component<sge::quad_component>();
        quad.position = position;
        quad.size = size;
        quad.color = color;
    }

    class sandbox_layer : public sge::layer {
    public:
        sandbox_layer() : layer("Sandbox Layer") {
            this->m_scene = sge::ref<sge::scene>::create();
            add_quad_to_scene(this->m_scene, "quad 1", glm::vec2(-1.f, -1.f), glm::vec2(1.f),
                              glm::vec4(0.f, 1.f, 0.f, 1.f));
            add_quad_to_scene(this->m_scene, "quad 2", glm::vec2(-1.f, 0.f), glm::vec2(1.f),
                              glm::vec4(0.f, 0.f, 1.f, 1.f));
            add_quad_to_scene(this->m_scene, "quad 3", glm::vec2(0.f, -1.f), glm::vec2(1.f),
                              glm::vec4(1.f, 1.f, 0.f, 1.f));
            add_quad_to_scene(this->m_scene, "quad 4", glm::vec2(0.f, 0.f), glm::vec2(1.f),
                              glm::vec4(1.f, 0.f, 0.f, 1.f));
        }

        virtual void on_event(sge::event& event) override {
            sge::event_dispatcher dispatcher(event);

            dispatcher.dispatch<sge::window_resize_event>(
                SGE_BIND_EVENT_FUNC(sandbox_layer::on_resize));
        }

        virtual void on_update(sge::timestep ts) override {
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
    };

    class sandbox_app : public sge::application {
    public:
        sandbox_app() : application("Sandbox") {}

    protected:
        virtual void init_app() override {
            this->m_layer = new sandbox_layer;
            this->push_layer(this->m_layer);

            auto image_data = sge::image_data::load("assets/images/tux.png");
            this->m_tux = sge::image_2d::create(image_data, sge::image_usage_texture);
        }

        virtual void shutdown_app() override {
            this->m_tux.reset();

            this->pop_layer(this->m_layer);
            delete this->m_layer;
        }

        sandbox_layer* m_layer;
        sge::ref<sge::image_2d> m_tux;
    };
} // namespace sandbox

sge::application* create_app_instance() { return new sandbox::sandbox_app; };