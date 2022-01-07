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
#include "texture_cache.h"
namespace sgm {
    struct cache_data_t {
        std::vector<std::unordered_set<ref<texture_2d>>> used_textures;
    };
    std::unique_ptr<cache_data_t> cache_data;

    void texture_cache::init() {
        cache_data = std::make_unique<cache_data_t>();

        swapchain& swap_chain = application::get().get_swapchain();
        size_t image_count = swap_chain.get_image_count();
        cache_data->used_textures.resize(image_count);
    }

    void texture_cache::shutdown() { cache_data.reset(); }

    void texture_cache::new_frame() {
        swapchain& swap_chain = application::get().get_swapchain();
        size_t current_image = swap_chain.get_current_image_index();
        cache_data->used_textures[current_image].clear();
    }

    void texture_cache::add_texture(ref<texture_2d> texture) {
        swapchain& swap_chain = application::get().get_swapchain();
        size_t current_image = swap_chain.get_current_image_index();
        cache_data->used_textures[current_image].insert(texture);
    }
} // namespace sgm
