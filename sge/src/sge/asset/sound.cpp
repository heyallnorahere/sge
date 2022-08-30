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
        void* decoder = nullptr;

        float duration = 0.f;
        ma_uint64 frame_count = 0;

        ma_result (*read)(void*, void*, size_t, size_t*) = nullptr;
        void (*destroy)(void*) = nullptr;
        void (*seekg)(void*, size_t) = nullptr;
        size_t (*tellg)(void*) = nullptr;
        bool (*query_data)(sound_asset_data_t*) = nullptr;
    };

    struct playing_sound_data_t {
        ref<sound> _asset;
        sound_asset_data_t* data;

        bool repeat;
        size_t current_frame;

        std::shared_ptr<sound_controller> controller;
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
    static size_t mix_frames(playing_sound_data_t& data, float* output, size_t frame_count) {
        static constexpr size_t buffer_size = 4096;
        size_t temp_frame_cap = buffer_size / s_sound_data->channel_count;

        float* temp_buffer = (float*)malloc(buffer_size * sizeof(float));
        if (temp_buffer == nullptr) {
            throw std::runtime_error("failed to allocate memory on the heap!");
        }

        data.data->seekg(data.data->decoder, data.current_frame);

        size_t total_frames_read = 0;
        while (total_frames_read < frame_count || !data.controller->is_stopped()) {
            data.current_frame = data.data->tellg(data.data->decoder);
            if (data.current_frame >= data.data->frame_count) {
                data.current_frame = 0;
                data.data->seekg(data.data->decoder, data.current_frame);
            }

            size_t total_remaining_frames = frame_count - total_frames_read;
            size_t frames_to_read = std::min(total_remaining_frames, temp_frame_cap);

            if (frames_to_read == 0) {
                break;
            }

            size_t frames_read;
            auto result =
                data.data->read(data.data->decoder, temp_buffer, frames_to_read, &frames_read);

            if ((result != MA_SUCCESS || frames_read == 0) && !data.repeat) {
                break;
            }

            for (size_t i = 0; i < frames_read * s_sound_data->channel_count; i++) {
                output[total_frames_read * s_sound_data->channel_count + i] += temp_buffer[i];
            }

            total_frames_read += frames_read;
            if (frames_read < frames_to_read && !data.repeat) {
                break;
            }
        }

        data.current_frame = data.data->tellg(data.data->decoder);
        free(temp_buffer);

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
            bool invalid_current_frame = playing_sound.current_frame == 0 ||
                 playing_sound.current_frame >= playing_sound.data->frame_count;

            bool stopped = read_frames < frame_count;
            stopped |= playing_sound.controller->is_stopped();
            stopped |= !playing_sound.repeat && invalid_current_frame;

            if (stopped) {
                size_t iterator_index = 0;
                for (auto it_ = s_sound_data->playing_sounds.begin(); it_ != it; it_++) {
                    iterator_index++;
                }

                s_sound_data->playing_sounds.erase(it);
                if (s_sound_data->playing_sounds.empty()) {
                    break;
                } else {
                    it = s_sound_data->playing_sounds.begin();
                    std::advance(it, iterator_index);
                }
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

    std::weak_ptr<sound_controller> sound::play(ref<sound> _sound, bool repeat) {
        mutex_lock lock(s_sound_data->sound_thread_mutex);
        auto& to_push = s_sound_data->playing_sounds.emplace_back();

        to_push._asset = _sound;
        to_push.data = _sound->m_data;

        to_push.repeat = repeat;
        to_push.current_frame = 0;

        to_push.controller = std::shared_ptr<sound_controller>(new sound_controller);
        return to_push.controller;
    }

    bool sound::stop(std::weak_ptr<sound_controller> controller) {
        if (auto ptr = controller.lock()) {
            if (!ptr->m_stopped) {
                ptr->m_stopped = true;
                return true;
            }
        }

        return false;
    }

    bool sound::stop_all() {
        mutex_lock lock(s_sound_data->sound_thread_mutex);

        bool result = false;
        for (const auto& playing_sound : s_sound_data->playing_sounds) {
            playing_sound.controller->m_stopped = true;
            result = true;
        }

        return result;
    }

    static ma_result read_audio_decoder(void* decoder, void* output, size_t frame_count,
                                        size_t* frames_read) {
        ma_uint64 read_frame_count;
        auto result = ma_decoder_read_pcm_frames((ma_decoder*)decoder, output,
                                                 (ma_uint64)frame_count, &read_frame_count);

        *frames_read = (size_t)read_frame_count;
        return result;
    }

    static void destroy_audio_decoder(void* decoder) {
        auto audio_decoder = (ma_decoder*)decoder;

        ma_decoder_uninit(audio_decoder);
        delete audio_decoder;
    }

    static void seekg_audio_decoder(void* decoder, size_t frame) {
        ma_decoder_seek_to_pcm_frame((ma_decoder*)decoder, (ma_uint64)frame);
    }

    static size_t tellg_audio_decoder(void* decoder) {
        auto audio_decoder = ((ma_decoder*)decoder);

        // i am almost certain this is a misuse of the api
        return (size_t)audio_decoder->readPointerInPCMFrames;
    }

    static bool query_audio_decoder(sound_asset_data_t* data) {
        auto decoder = (ma_decoder*)data->decoder;
        auto data_source = decoder->pBackend;

        if (ma_data_source_get_length_in_seconds(data_source, &data->duration) != MA_SUCCESS) {
            return false;
        }

        if (ma_data_source_get_length_in_pcm_frames(data_source, &data->frame_count) !=
            MA_SUCCESS) {
            return false;
        }

        // todo: query for more data

        return true;
    }

    bool sound::reload() {
        // todo(nora): destructive on fail, please fix
        sound_asset_data_t* data = nullptr;
        if (m_path.extension() != ".ogg") {
            auto decoder = new ma_decoder;

            std::string string_path = m_path.string();
            if (ma_decoder_init_file(string_path.c_str(), &s_sound_data->decoder_config, decoder) !=
                MA_SUCCESS) {
                delete decoder;
                return false;
            }

            data = new sound_asset_data_t;
            data->decoder = decoder;

            data->read = read_audio_decoder;
            data->destroy = destroy_audio_decoder;

            data->seekg = seekg_audio_decoder;
            data->tellg = tellg_audio_decoder;

            data->query_data = query_audio_decoder;
        } else {
            // todo: vorbis
            return false;
        }

        if (data == nullptr || data->query_data == nullptr || !data->query_data(data)) {
            if (data != nullptr) {
                data->destroy(data->decoder);
                delete data;
            }

            return false;
        }

        cleanup();
        m_data = data;

        return true;
    }

    float sound::get_duration() { return m_data->duration; }

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