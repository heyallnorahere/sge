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
#include "sge/core/application.h"
#include "sge/renderer/renderer.h"
#include "sge/core/input.h"
#include "sge/imgui/imgui_layer.h"
#include "sge/script/script_engine.h"
#include "sge/asset/asset_serializers.h"
#include "sge/asset/project.h"
#include "sge/asset/sound.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

extern sge::application* create_app_instance();
namespace sge {
    int32_t application::entrypoint::operator()(int32_t argc, const char** argv) {
#ifndef SGE_DEBUG
        try {
#endif
            sge::application::create();

            std::vector<std::string> args;
            for (int32_t i = 0; i < argc; i++) {
                args.push_back(argv[i]);
            }

            auto& app = sge::application::get();
            app.set_application_args(args);

            app.init();
            app.run();
            app.shutdown();

            sge::application::destroy();
            return EXIT_SUCCESS;
#ifndef SGE_DEBUG
        } catch (const std::exception& exc) {
            spdlog::error(exc.what());
            return EXIT_FAILURE;
        }
#endif
    }

    std::string application::get_engine_version() { return SGE_VERSION; }

    static std::unique_ptr<application> s_app_instance;
    void application::create() {
        s_app_instance = std::unique_ptr<application>(::create_app_instance());
    }

    void application::destroy() { s_app_instance.reset(); }

    application& application::get() { return *s_app_instance; }

    application::application(const std::string& title) {
        m_title = title;
        m_running = false;
        m_minimized = false;
    }

    void application::on_event(event& e) {
        event_dispatcher dispatcher(e);

        dispatcher.dispatch<window_close_event>(SGE_BIND_EVENT_FUNC(application::on_window_close));
        dispatcher.dispatch<window_resize_event>(
            SGE_BIND_EVENT_FUNC(application::on_window_resize));

        if (is_subsystem_initialized(subsystem_input)) {
            input::on_event(e);
        }

        for (auto& _layer : m_layer_stack) {
            if (e.handled) {
                break;
            }
            _layer->on_event(e);
        }
    }

    bool application::is_watching(const fs::path& path) {
        if (path.empty()) {
            return false;
        }

        for (const auto& [directory, watcher] : m_watchers) {
            auto relative_path = fs::relative(path, directory);
            if (relative_path.empty() || *relative_path.begin() != "..") {
                return true;
            }
        }

        return false;
    }

    bool application::watch_directory(const fs::path& path) {
        if (is_watching(path)) {
            return false;
        }

        if (path.empty()) {
            return false;
        }

        m_watchers[path] = std::make_unique<directory_watcher>(path);
        return true;
    }

    bool application::remove_watched_directory(const fs::path& path) {
        if (m_watchers.find(path) == m_watchers.end()) {
            return false;
        }

        m_watchers.erase(path);
        return true;
    }

    struct imgui_app_data {
        std::string config_path;
    };

    void application::init() {
        init_logger();

        spdlog::info("using SGE v{0}", get_engine_version());
        spdlog::info("initializing application: {0}...", m_title);

        pre_init();
        if ((m_disabled_subsystems & subsystem_input) == 0) {
            input::init();
            m_initialized_subsystems |= subsystem_input;
        }

        m_window = window::create(get_window_title(), 1600, 900);
        m_window->set_event_callback(SGE_BIND_EVENT_FUNC(application::on_event));

        renderer::init();
        m_swapchain = swapchain::create(m_window);

        if ((m_disabled_subsystems & subsystem_asset) == 0) {
            asset_serializer::init();
            m_initialized_subsystems |= subsystem_asset;

            if ((m_disabled_subsystems & subsystem_script_engine) == 0) {
                script_engine::init();
                m_initialized_subsystems |= subsystem_script_engine;
            }
        }

        m_imgui_layer = new imgui_layer;
        push_overlay(m_imgui_layer);

        ImGuiIO& io = ImGui::GetIO();
        auto imgui_data = new imgui_app_data;
        io.UserData = imgui_data;

        fs::path config_path = get_imgui_config_path();
        if (!config_path.empty()) {
            imgui_data->config_path = config_path.string();
            io.IniFilename = imgui_data->config_path.c_str();
        }

        if ((m_disabled_subsystems &
             (subsystem_asset | subsystem_script_engine | subsystem_project)) == 0) {
            project::init();
            m_initialized_subsystems |= subsystem_project;
        }

        if ((m_disabled_subsystems & subsystem_sound) == 0) {
            sound::init();
            m_initialized_subsystems |= subsystem_sound;
        }

        on_init();
    }

    void application::shutdown() {
        spdlog::info("shutting down application: {0}...", m_title);

        renderer::clear_render_data();
        on_shutdown();

        if (is_subsystem_initialized(subsystem_sound)) {
            sound::shutdown();
        }

        if (is_subsystem_initialized(subsystem_project)) {
            project::shutdown();
        }

        if (is_subsystem_initialized(subsystem_script_engine)) {
            script_engine::shutdown();
        }

        ImGuiIO& io = ImGui::GetIO();
        auto imgui_data = (imgui_app_data*)io.UserData;

        pop_overlay(m_imgui_layer);
        delete m_imgui_layer;
        delete imgui_data;
        m_imgui_layer = nullptr;

        m_swapchain.reset();
        renderer::shutdown();

        m_window.reset();
        if (is_subsystem_initialized(subsystem_input)) {
            input::shutdown();
        }

        spdlog::shutdown();
    }

    void application::run() {
        if (m_running) {
            throw std::runtime_error("cannot recursively call run()");
        }

        m_running = true;
        while (m_running) {
            for (const auto& [path, watcher] : m_watchers) {
                watcher->update();
                watcher->process_events(SGE_BIND_EVENT_FUNC(application::on_event));
            }

            if (!m_minimized) {
                m_swapchain->new_frame();
                renderer::new_frame();

                size_t current_image = m_swapchain->get_current_image_index();
                auto& cmdlist = m_swapchain->get_command_list(current_image);
                cmdlist.begin();
                renderer::set_command_list(cmdlist);

                auto pass = m_swapchain->get_render_pass();
                renderer::push_render_pass(pass, glm::vec4(0.3f, 0.3f, 0.3f, 1.f));

                timestep ts;
                {
                    using namespace std::chrono;

                    static high_resolution_clock clock;
                    static high_resolution_clock::time_point t0 = clock.now();
                    high_resolution_clock::time_point t1 = clock.now();

                    ts = duration_cast<timestep>(t1 - t0);
                    t0 = t1;
                }

                for (auto it = m_layer_stack.rbegin(); it != m_layer_stack.rend(); it++) {
                    (*it)->on_update(ts);
                }

                renderer::begin_render_pass();
                m_imgui_layer->begin();
                for (auto& layer : m_layer_stack) {
                    layer->on_imgui_render();
                }
                m_imgui_layer->end(cmdlist);

                if (renderer::pop_render_pass() != pass) {
                    throw std::runtime_error("a render pass was pushed, but not popped!");
                }
                cmdlist.end();

                m_swapchain->present();
            }

            m_window->on_update();
        }
    }

    void application::init_logger() {
        spdlog::level::level_enum logger_level;
#ifdef SGE_DEBUG
        logger_level = spdlog::level::debug;
#else
        logger_level = spdlog::level::info;
#endif

        auto color_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        std::vector<spdlog::sink_ptr> sinks = { color_sink };

        auto log_path = get_log_file_path();
        if (!log_path.empty()) {
            std::string path = log_path.string();

            auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path, true);
            file_sink->set_level(spdlog::level::debug);

            sinks.push_back(file_sink);
        }

        auto logger = std::make_shared<spdlog::logger>("SGE logger", sinks.begin(), sinks.end());
        logger->set_pattern("[%m-%d-%Y %T] [" + m_title + "] [%^%l%$] %v");
        logger->set_level(logger_level);

        spdlog::set_default_logger(logger);
    }

    void application::disable_subsystem(subsystem id) { m_disabled_subsystems |= id; }
    void application::reenable_subsystem(subsystem id) { m_disabled_subsystems &= ~id; }

    bool application::on_window_resize(window_resize_event& e) {
        uint32_t width = e.get_width();
        uint32_t height = e.get_height();

        m_minimized = (width == 0 || height == 0);
        if (!m_minimized) {
            m_swapchain->on_resize(width, height);
        }

        return false;
    }

    bool application::on_window_close(window_close_event& e) {
        m_running = false;
        return true;
    }
} // namespace sge
