/*
   Copyright 2021 Nora Beda

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
#include "sge/renderer/command_queue.h"
#include "sge/core/window.h"
namespace sge {
    class renderer_api {
    public:
        virtual ~renderer_api() = default;

        virtual void init() = 0;
        virtual void shutdown() = 0;
    };
    class renderer {
    public:
        renderer() = delete;

        static void init();
        static void shutdown();

        static ref<command_queue> get_queue(command_list_type type);
    };
} // namespace sge