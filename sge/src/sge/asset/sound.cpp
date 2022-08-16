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
        void* decoder;
        bool (*read)(void*, void*, size_t, size_t*);
        void (*destroy)(void*);
    };

    struct playing_sound_data_t {
        ref<sound> _asset;
        sound_asset_data_t* data;

        bool repeat;
        size_t current_sample;
    };

    struct sound_data_t {
        ma_format format;
        ma_uint32 channel_count;
        ma_uint32 sample_rate;

        ma_device device;
        ma_decoder_config decoder_config;

        std::list<playing_sound_data_t> playing_sounds;
        std::mutex sound_thread_mutex;
    };

    class mutex_lock {
    public:
        mutex_lock(std::mutex& mutex) : m_mutex(mutex) { m_mutex.lock(); }
        ~mutex_lock() { m_mutex.unlock(); }

        mutex_lock(const mutex_lock&) = delete;
        mutex_lock& operator=(const mutex_lock&) = delete;
    
    private:
        std::mutex& m_mutex;
    };

    static std::unique_ptr<sound_data_t> s_sound_data;
    static size_t mix_frames(const playing_sound_data_t& data, float* output, size_t frame_count) {
        static constexpr size_t buffer_size = 4096;

        float temp_buffer[buffer_size];
        size_t temp_frame_cap = buffer_size / s_sound_data->channel_count;

        size_t total_frames_read = 0;
        while (total_frames_read < frame_count) {
            size_t total_remaining_frames = frame_count - total_frames_read;
            size_t frames_to_read = std::min(total_remaining_frames, temp_frame_cap);

            size_t frames_read;
            if (!data.data->read(data.data->decoder, temp_buffer, frames_to_read, &frames_read)) {
                break;
            }

            for (size_t i = 0; i < frames_read * s_sound_data->channel_count; i++) {
                output[total_frames_read * s_sound_data->channel_count + i] = temp_buffer[i];
            }

            total_frames_read += frames_read;
            if (frames_read < frames_to_read) {
                break;
            }
        }

        return total_frames_read;
    }

    static void sound_data_callback(ma_device* device, void* output, const void* input,
                                    ma_uint32 frame_count) {
        mutex_lock lock(s_sound_data->sound_thread_mutex);
        if (device->playback.format != s_sound_data->format) {
            throw std::runtime_error("mismatched sample format!");
        }

        float* output_buffer = (float*)output;
        for (auto it = s_sound_data->playing_sounds.begin();
             it != s_sound_data->playing_sounds.end(); it++) {
            auto& playing_sound = *it;

            size_t read_frames = mix_frames(playing_sound, output_buffer, frame_count);
            if (read_frames < frame_count) {
                auto pos = it;

                // hack hackity hack hack
                it++;
                s_sound_data->playing_sounds.erase(pos);
                it--;
            }
        }
    }

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
        mutex_lock lock(s_sound_data->sound_thread_mutex);
        auto& to_push = s_sound_data->playing_sounds.emplace_back();

        to_push._asset = _sound;
        to_push.data = _sound->m_data;

        to_push.repeat = repeat;
        to_push.current_sample = 0;
    }

    static bool read_audio_decoder(void* decoder, void* output, size_t frame_count,
                                   size_t* frames_read) {
        ma_uint64 read_frame_count;
        auto result = ma_decoder_read_pcm_frames((ma_decoder*)decoder, output,
                                                 (ma_uint64)frame_count, &read_frame_count);

        if (result != MA_SUCCESS || read_frame_count == 0) {
            return false;
        }

        *frames_read = (size_t)read_frame_count;
        return true;
    }

    static void destroy_audio_decoder(void* decoder) {
        auto audio_decoder = (ma_decoder*)decoder;
        
        ma_decoder_uninit(audio_decoder);
        delete audio_decoder;
    }

    bool sound::reload() {
        sound_asset_data_t* data;
        if (m_path.extension() != ".ogg") {
            auto decoder = new ma_decoder;

            std::string string_path = m_path.string();
            if (ma_decoder_init_file(string_path.c_str(), &s_sound_data->decoder_config,
                                     decoder) != MA_SUCCESS) {
                delete decoder;
                return false;
            }

            data = new sound_asset_data_t;
            data->decoder = decoder;
            data->read = read_audio_decoder;
            data->destroy = destroy_audio_decoder;
        }
        
        // todo: vorbis

        cleanup();
        m_data = data;

        return true;
    }

    void sound::load() {
        if (!reload()) {
            throw std::runtime_error("Failed to load sound!");
        }
    }

    void sound::cleanup() {
        if (m_data == nullptr) {
            return;
        }

        m_data->destroy(m_data->decoder);
        delete m_data;

        m_data = nullptr;
    }
} // namespace sge