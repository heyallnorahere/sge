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

#include "sgepch.h"
#include "sge/renderer/renderer.h"
#ifdef SGE_USE_VULKAN
#include "sge/platform/vulkan/vulkan_renderer.h"
#endif
namespace sge {
    static struct {
        std::unique_ptr<renderer_api> api;
        std::map<command_list_type, ref<command_queue>> queues;
    } renderer_data;

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
    }

    void renderer::shutdown() {
        renderer_data.queues.clear();

        renderer_data.api->shutdown();
        renderer_data.api.reset();
    }

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
}