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
#include "sge/platform/desktop/desktop_window.h"
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
namespace sge {
    std::optional<fs::path> desktop_window::native_file_dialog(
        dialog_mode mode, const std::vector<dialog_file_filter>& filters) {
        fs::path zenity_path = "/usr/bin/zenity";
        if (fs::exists(zenity_path)) {
            std::stringstream command;
            command << zenity_path.string() + " --file-selection";

            if (mode == dialog_mode::save) {
                command << " --save --confirm-overwrite";
            }

            for (const auto& filter : filters) {
                std::string filter_arg;
                if (filter.name.empty()) {
                    filter_arg = filter.filter;
                } else {
                    filter_arg = filter.name + " | " + filter.filter;
                }

                command << " --file-filter=" << std::quoted(filter_arg);
            }

            std::string string_command = command.str();
            FILE* pipe = popen(string_command.c_str(), "r");
            if (pipe == nullptr) {
                spdlog::warn("could not open zenity");
                return std::optional<fs::path>();
            }
            
            static constexpr size_t buffer_size = 256;
            char buffer[buffer_size];
            std::stringstream result;

            try {
                size_t bytes_read;
                while ((bytes_read = fread(buffer, sizeof(char), buffer_size, pipe)) != 0) {
                    result << std::string(buffer, bytes_read);
                }
            } catch (...) {
                pclose(pipe);
                spdlog::warn("exception thrown reading zenity pipe");
                return std::optional<fs::path>();
            }

            if (pclose(pipe) == 0) {
                std::string result_string = result.str();

                size_t pos;
                while ((pos = result_string.find('\n')) != std::string::npos) {
                    result_string.replace(pos, 1, std::string());
                }

                return result_string;
            }
        } else {
            spdlog::warn("could not find zenity at {0}", zenity_path.string());
        }

        return std::optional<fs::path>();
    }
}