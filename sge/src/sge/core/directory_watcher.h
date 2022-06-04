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
#include "sge/events/event.h"

namespace sge {
    class directory_watcher {
    public:
        directory_watcher(const fs::path& directory);
        ~directory_watcher() = default;

        directory_watcher(const directory_watcher&) = delete;
        directory_watcher& operator=(const directory_watcher&) = delete;

        void update();

        template <typename Func>
        void process_events(Func&& callback) {
            while (!m_unhandled_events.empty()) {
                const auto& data = m_unhandled_events.front();
                file_changed_event e(data.path, m_directory, data.status);
                
                callback(e);
                m_unhandled_events.pop();
            }
        }
        
    private:
        struct unhandled_event {
            fs::path path;
            file_status status;
        };

        void submit_event(const fs::path& path, file_status status);

        fs::path m_directory;
        std::unordered_map<fs::path, fs::file_time_type, path_hasher> m_paths;
        std::queue<unhandled_event> m_unhandled_events;
    };
} // namespace sge