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
#include "sge/renderer/image.h"
#ifdef SGE_USE_VULKAN
#include "sge/platform/vulkan/vulkan_base.h"
#include "sge/platform/vulkan/vulkan_image.h"
#endif
#ifdef SGE_PLATFORM_LINUX
#define STBI_NO_SIMD
#endif
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
namespace sge {
    std::unique_ptr<image_data> image_data::load(const fs::path& path) {
        std::string string_path = path.string();
        if (!fs::exists(path)) {
            throw std::runtime_error("image " + string_path + " does not exist!");
        }

        int32_t width, height, channels;
        uint8_t* data = stbi_load(string_path.c_str(), &width, &height, &channels, 0);
        if (data == nullptr) {
            return std::unique_ptr<image_data>(nullptr);
        }

        image_format format;
        switch (channels) {
        case 3:
            format = image_format::RGB8_SRGB;
            break;
        case 4:
            format = image_format::RGBA8_SRGB;
            break;
        default:
            throw std::runtime_error("invalid image format!");
        }

        size_t data_size = (size_t)width * height * channels;
        auto img_data = create(data, data_size, width, height, format);

        stbi_image_free(data);
        return std::move(img_data);
    }

    std::unique_ptr<image_data> image_data::create(const void* data, size_t size, uint32_t width,
                                                   uint32_t height, image_format format) {
        auto img_data = std::unique_ptr<image_data>(new image_data);

        img_data->m_size = size;
        img_data->m_width = width;
        img_data->m_height = height;
        img_data->m_format = format;

        img_data->m_data = malloc(size);
        memcpy(img_data->m_data, data, size);

        return std::move(img_data);
    }

    image_data::~image_data() { free(this->m_data); }

    ref<image_2d> image_2d::create(const std::unique_ptr<image_data>& data, uint32_t additional_usage) {
        image_spec spec;
        spec.width = data->get_width();
        spec.height = data->get_height();
        spec.format = data->get_format();
        spec.image_usage = image_usage_texture | additional_usage;
        spec.array_layers = 1;

        // todo(nora): compute mip levels

        auto img = create(spec);
        img->copy_from(data->get_data(), data->get_data_size());
        return img;
    }

    ref<image_2d> image_2d::create(const image_spec& spec) {
#ifdef SGE_USE_VULKAN
        return ref<vulkan_image_2d>::create(spec);
#endif

        return nullptr;
    }
} // namespace sge