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
#include "sge/asset/sound.h"

// todo: maybe link libvorbis if its available?
#define MA_NO_LIBVORBIS

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
#include <extras/miniaudio_libvorbis.h>

namespace sge {
    using sound_clock = std::chrono::high_resolution_clock;
    using sound_timestamp = sound_clock::time_point;

    struct sound_asset_data_t {
        ma_decoder decoder;
    };

    struct playing_sound_data_t {
        ref<sound> _asset;
        sound_asset_data_t* data;

        bool repeat;
        sound_timestamp start;
    };

    struct sound_data_t {
        ma_format format;
        ma_uint32 channel_count;
        ma_uint32 sample_rate;

        ma_device device;
        ma_decoder_config decoder_config;

        std::list<playing_sound_data_t> playing_sounds;
    };

    static std::unique_ptr<sound_data_t> s_sound_data;
    static void sound_data_callback(ma_device* device, void* output, const void* input,
                                    ma_uint32 frame_count) {}

    void sound::init() {
        if (s_sound_data) {
            throw std::runtime_error("the sound subsystem has already been initialized!");
        }

        s_sound_data = std::make_unique<sound_data_t>();
        s_sound_data->format = ma_format_f32;
        s_sound_data->channel_count = 2;
        s_sound_data->sample_rate = 48000;

        s_sound_data->decoder_config = ma_decoder_config_init(
            s_sound_data->format, s_sound_data->channel_count, s_sound_data->sample_rate);

        auto device_config = ma_device_config_init(ma_device_type_playback);
        device_config.playback.format = s_sound_data->format;
        device_config.playback.channels = s_sound_data->channel_count;
        device_config.sampleRate = s_sound_data->sample_rate;
        device_config.dataCallback = sound_data_callback;
        device_config.pUserData = nullptr;

        if (ma_device_init(nullptr, &device_config, &s_sound_data->device) != MA_SUCCESS) {
            throw std::runtime_error("Failed to initialize audio device!");
        }

        if (ma_device_start(&s_sound_data->device) != MA_SUCCESS) {
            ma_device_uninit(&s_sound_data->device);

            throw std::runtime_error("Failed to start audio device!");
        }
    }

    void sound::shutdown() {
        if (!s_sound_data) {
            throw std::runtime_error("the sound subsystem is not initialized!");
        }

        ma_device_uninit(&s_sound_data->device);
        s_sound_data.reset();
    }

    void sound::play(ref<sound> _sound, bool repeat) {
        auto& to_push = s_sound_data->playing_sounds.emplace_back();

        to_push._asset = _sound;
        to_push.data = _sound->m_data;

        to_push.repeat = repeat;
        to_push.start = sound_clock::now();
    }

    bool sound::reload() {
        auto data = new sound_asset_data_t;

        std::string string_path = m_path.string();
        if (ma_decoder_init_file(string_path.c_str(), &s_sound_data->decoder_config,
                                 &data->decoder) != MA_SUCCESS) {
            delete data;
            return false;
        }

        cleanup();
        m_data = data;

        return true;
    }

    void sound::cleanup() {
        if (m_data == nullptr) {
            return;
        }

        ma_decoder_uninit(&m_data->decoder);
        delete m_data;

        m_data = nullptr;
    }
} // namespace sge