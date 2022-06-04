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
#include "sge/core/directory_watcher.h"

namespace sge {
    directory_watcher::directory_watcher(const fs::path& directory) : m_directory(directory) {
        if (!fs::exists(m_directory)) {
            throw std::runtime_error("The passed directory does not exist!");
        }

        if (!fs::is_directory(m_directory)) {
            throw std::runtime_error("The passed path is not a directory!");
        }

        for (const auto& entry : fs::recursive_directory_iterator(m_directory)) {
            auto last_write_time = fs::last_write_time(entry);
            const auto& path = entry.path();

            m_paths.insert(std::make_pair(path, last_write_time));
        }
    }

    void directory_watcher::update() {
        std::vector<fs::path> to_erase;
        for (const auto& [path, time] : m_paths) {
            if (!fs::exists(path)) {
                submit_event(path, file_status::deleted);
                to_erase.push_back(path);
            }
        }

        for (const auto& path : to_erase) {
            m_paths.erase(path);
        }

        for (const auto& entry : fs::recursive_directory_iterator(m_directory)) {
            try {
                auto last_write_time = fs::last_write_time(entry);
                const auto& path = entry.path();

                if (m_paths.find(path) == m_paths.end()) {
                    m_paths.insert(std::make_pair(path, last_write_time));
                    submit_event(path, file_status::created);
                } else if (m_paths.at(path) < last_write_time) {
                    m_paths[path] = last_write_time;
                    submit_event(path, file_status::modified);
                }
            } catch (const fs::filesystem_error&) {
                continue;
            }
        }
    }

    void directory_watcher::submit_event(const fs::path& path, file_status status) {
        unhandled_event e;
        e.path = path;
        e.status = status;

        m_unhandled_events.push(e);
    }
} // namespace sge