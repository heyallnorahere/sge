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

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace sge {
    std::unique_ptr<image_data> image_data::load(const fs::path& path) {
        std::string string_path = path.string();
        if (!fs::exists(path)) {
            throw std::runtime_error("image " + string_path + " does not exist!");
        }

        int x, y, comp;
        uint8_t* data = stbi_load(string_path.c_str(), &x, &y, &comp, 0);
        if (data == nullptr) {
            return std::unique_ptr<image_data>(nullptr);
        }

        uint32_t width = (uint32_t)x;
        uint32_t height = (uint32_t)y;

        image_format format;
        switch (comp) {
        case 3:
            format = image_format::RGB8_SRGB;
            break;
        case 4:
            format = image_format::RGBA8_SRGB;
            break;
        default:
            throw std::runtime_error("invalid image format!");
        }

        size_t data_size = (size_t)comp * width * height;
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

    image_data::~image_data() { free(m_data); }

    static int write_png(const char* path, int x, int y, int comp, const void* data) {
        int stride = x * comp;
        return stbi_write_png(path, x, y, comp, data, stride);
    }

    static int write_jpeg(const char* path, int x, int y, int comp, const void* data) {
        return stbi_write_jpg(path, x, y, comp, data, 100);
    }

    using write_callback_t = int (*)(const char*, int, int, int, const void*);
    bool image_data::write(const fs::path& path) const {
        static std::unordered_map<fs::path, write_callback_t, path_hasher> extension_types;
        if (extension_types.empty()) {
            auto register_format = [&](const std::vector<fs::path>& extensions,
                                       write_callback_t callback) {
                for (const auto& extension : extensions) {
                    extension_types.insert(std::make_pair(extension, callback));
                }
            };

            register_format({ ".png" }, write_png);
            register_format({ ".bmp" }, stbi_write_bmp);
            register_format({ ".tga" }, stbi_write_tga);
            register_format({ ".jpeg", ".jpg" }, write_jpeg);
        }

        if (!path.has_extension()) {
            return false;
        }

        fs::path extension = path.extension();
        if (extension_types.find(extension) == extension_types.end()) {
            return false;
        }

        std::string string_path = path.string();
        const char* c_str = string_path.c_str();

        int x = (int)m_width;
        int y = (int)m_height;
        int comp = (int)m_size / (x * y);

        auto callback = extension_types.at(extension);
        return callback(c_str, x, y, comp, m_data) == 1;
    }

    uint32_t image_2d::get_channel_count(image_format format) {
        switch (format) {
        case image_format::RGB8_UINT:
        case image_format::RGB8_SRGB:
            return 3;
        case image_format::RGBA8_UINT:
        case image_format::RGBA8_SRGB:
            return 4;
        default:
            return 0;
        }
    }

    ref<image_2d> image_2d::create(const std::unique_ptr<image_data>& data,
                                   uint32_t additional_usage) {
        image_spec spec;
        spec.width = data->get_width();
        spec.height = data->get_height();
        spec.format = data->get_format();
        spec.image_usage = image_usage_transfer | additional_usage;
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

    std::unique_ptr<image_data> image_2d::dump() {
        uint32_t width = get_width();
        uint32_t height = get_height();
        image_format format = get_format();

        uint32_t channels = get_channel_count(format);
        size_t size = (size_t)width * height * channels;

        void* buffer = malloc(size);
        std::unique_ptr<image_data> result;

        if (buffer != nullptr && copy_to(buffer, size)) {
            result = image_data::create(buffer, size, width, height, format);
        }

        free(buffer);
        return std::move(result);
    }
} // namespace sge