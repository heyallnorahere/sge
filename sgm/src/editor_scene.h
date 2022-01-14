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
#include <sge/renderer/framebuffer.h>
namespace sgm {
    class editor_scene {
    public:
        editor_scene() = delete;

        static void create();
        static void destroy();

        static void on_update(timestep ts);
        static void on_event(event& e);

        static void set_viewport_size(uint32_t width, uint32_t height);

        static entity& get_selection();
        static void reset_selection();

        static void enable_input();
        static void disable_input();

        static void load(const fs::path& path);
        static void save(const fs::path& path);

        static bool running();
        static void play();
        static void stop();

        static size_t get_assembly_index();
        static ref<scene> get_scene();
        static ref<framebuffer> get_framebuffer();
        static editor_camera& get_camera();
    };
} // namespace sgm