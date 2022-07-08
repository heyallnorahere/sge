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
namespace sge {
    enum class image_format {
        RGB8_UINT,
        RGB8_SRGB,

        RGBA8_UINT,
        RGBA8_SRGB,
    };

    enum image_usage {
        image_usage_none = 0x0,
        image_usage_texture = 0x1,
        image_usage_attachment = 0x2,
        image_usage_storage = 0x4,
        image_usage_transfer = 0x8,
    };

    /*enum class image_mode {
        undefined,
        sampled,
        storage,
        attachment
    };*/

    class image_data {
    public:
        static std::unique_ptr<image_data> load(const fs::path& path);
        static std::unique_ptr<image_data> create(const void* data, size_t size, uint32_t width,
                                                  uint32_t height, image_format format);

        ~image_data();

        image_data(const image_data&) = delete;
        image_data operator=(const image_data&) = delete;

        const void* get_data() const { return m_data; }
        const size_t get_data_size() const { return m_size; }

        uint32_t get_width() const { return m_width; }
        uint32_t get_height() const { return m_height; }
        image_format get_format() const { return m_format; }

        bool write(const fs::path& path) const;

    private:
        image_data() = default;

        void* m_data;
        size_t m_size;
        uint32_t m_width, m_height;
        image_format m_format;
    };

    struct image_spec {
        image_format format = image_format::RGBA8_UINT;
        uint32_t image_usage = image_usage_none;

        uint32_t width = 1;
        uint32_t height = 1;

        uint32_t mip_levels = 1;
        uint32_t array_layers = 1;
    };

    class image_2d : public ref_counted {
    public:
        static uint32_t get_channel_count(image_format format);

        static ref<image_2d> create(const std::unique_ptr<image_data>& data,
                                    uint32_t additional_usage);
        static ref<image_2d> create(const image_spec& spec);

        virtual ~image_2d() = default;

        virtual uint32_t get_width() = 0;
        virtual uint32_t get_height() = 0;

        virtual uint32_t get_mip_level_count() = 0;
        virtual uint32_t get_array_layer_count() = 0;

        virtual image_format get_format() = 0;
        virtual uint32_t get_usage() = 0;

        std::unique_ptr<image_data> dump();

    protected:
        virtual void copy_from(const void* data, size_t size) = 0;
        virtual bool copy_to(void* data, size_t size) = 0;
    };
} // namespace sge