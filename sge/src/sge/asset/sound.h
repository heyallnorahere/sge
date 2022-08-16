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
#include "sge/asset/asset.h"
#include "sge/asset/json.h"

namespace sge {
    class sound_controller {
    public:
        ~sound_controller() = default;

        bool is_stopped() const { return m_stopped; }

    private:
        sound_controller() = default;

        bool m_stopped = false;
        friend class sound;
    };

    struct sound_asset_data_t;
    class sound : public asset {
    public:
        static void init();
        static void shutdown();

        static std::weak_ptr<sound_controller> play(ref<sound> _sound, bool repeat);
        static bool stop(std::weak_ptr<sound_controller> controller);
        static bool stop_all();

        sound(const fs::path& path) : m_path(path) { load(); }
        virtual ~sound() override { cleanup(); }

        sound(const sound&) = delete;
        sound& operator=(const sound&) = delete;

        virtual asset_type get_asset_type() override { return asset_type::sound; }
        virtual const fs::path& get_path() override { return m_path; }

        virtual bool reload() override;

        float get_duration();

    private:
        void load();
        void cleanup();

        fs::path m_path;
        sound_asset_data_t* m_data = nullptr;
    };
} // namespace sge