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
#include "sge/core/environment.h"
#include <iostream>

#ifdef SGE_PLATFORM_WINDOWS
#include "sge/platform/windows/windows_environment.h"
#else
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef SGE_PLATFORM_LINUX
#define getenv secure_getenv
#endif
#endif

namespace sge {
    int32_t environment::run_command(const process_info& info) {
        if (!info.output_file.empty()) {
            fs::path directory = info.output_file.parent_path();

            if (!fs::exists(directory)) {
                fs::create_directories(directory);
            } else if (!fs::is_directory(directory)) {
                throw std::runtime_error("whoops, " + directory.string() + " is not a directory");
            }
        }

#ifdef SGE_PLATFORM_WINDOWS
        return windows_run_command(info);
#else
        if (info.detach) {
            // shamelessly stolen from stack overflow
            // https://stackoverflow.com/questions/17954432/creating-a-daemon-in-linux

            // first fork
            pid_t pid = fork();
            if (pid < 0) {
                spdlog::error("could not perform first fork");
                return -1;
            }

            if (pid == 0) {
                if (setsid() < 0) {
                    spdlog::error("could not set session ID");
                    exit(EXIT_FAILURE);
                }

                signal(SIGCHLD, SIG_IGN);
                signal(SIGHUP, SIG_IGN);

                // second fork
                pid = fork();
                if (pid < 0) {
                    spdlog::error("could not perform second fork");
                    exit(EXIT_FAILURE);
                }

                if (pid > 0) {
                    exit(EXIT_SUCCESS);
                }

                umask(0);
                if (!info.workdir.empty()) {
                    fs::path workdir = fs::absolute(info.workdir);
                    spdlog::info("changing to directory: {0}", workdir.string());
                    chdir(workdir.c_str());
                }

                // close all file descriptors
                for (int32_t i = sysconf(_SC_OPEN_MAX); i >= 0; i--) {
                    close(i);
                }

                // open standard fds
                for (int32_t i = 0; i < 3; i++) {
                    std::string mode = std::string(i > 0 ? "w" : "r") + "b";

                    fs::path fd_path = "/dev/null";
                    if (i > 0 && !info.output_file.empty()) {
                        fd_path = info.output_file;
                    }

                    fopen(fd_path.c_str(), mode.c_str());
                }

                std::vector<std::string> args = {
                    "sh", "-c", info.cmdline
                };

                std::vector<char> data;
                std::vector<size_t> indices;
                for (const auto& arg : args) {
                    size_t index = data.size();
                    indices.push_back(index);

                    data.insert(data.end(), arg.begin(), arg.end());
                    data.push_back('\0');
                }

                std::vector<char*> arguments;
                for (size_t index : indices) {
                    arguments.push_back(&data[index]);
                }

                arguments.push_back(nullptr);
                int32_t return_code = execvp("/bin/sh", arguments.data());
                exit(return_code);
            }

            return 0;
        }

        std::string cmdline;
        if (!info.workdir.empty() && info.workdir != fs::current_path()) {
            std::stringstream stream;

            stream << "cd " << std::quoted(info.workdir.string()) << " && ";
            stream << info.cmdline;

            cmdline = stream.str();
        } else {
            cmdline = info.cmdline;
        }

        FILE* pipe = popen(cmdline.c_str(), "r");
        if (pipe == nullptr) {
            spdlog::error("could not run command: {0}", strerror(errno));
            return -1;
        }

        try {
            bool output_file = !info.output_file.empty();
            std::stringstream output;

            std::array<char, 256> buffer;
            while (fgets(buffer.data(), buffer.size(), pipe)) {
                std::string data = buffer.data();

                if (output_file) {
                    output << data;
                } else {
                    std::cout << data << std::flush;
                }
            }

            if (output_file) {
                std::ofstream file(info.output_file);
                file << output.str() << std::flush;
                file.close();
            }
        } catch (...) {
            pclose(pipe);
            throw;
        }

        return pclose(pipe);
#endif
    }

    static void export_variable_bourne(const std::string& key, const std::string& value,
                                       std::stringstream& stream) {
        stream << key << "=" << std::quoted(value) << "\nexport " << key;
    }

    static void export_variable_csh_tcsh(const std::string& key, const std::string& value,
                                         std::stringstream& stream) {
        stream << "setenv " << key << " " << std::quoted(value);
    }

    bool environment::set(const std::string& key, const std::string& value) {
#ifdef SGE_PLATFORM_WINDOWS
        return windows_setenv(key, value);
#else
        fs::path shell_path = get("SHELL");
        if (!shell_path.empty()) {
            std::string shell = shell_path.filename();

            if (shell == "fish") {
                std::stringstream command;
                command << "set -Ux " << key << " " << std::quoted(value);

                std::stringstream fish_command;
                fish_command << "fish -c " << std::quoted(command.str());

                std::string end_command = fish_command.str();
                if (system(end_command.c_str()) != 0) {
                    return false;
                }
            } else {
                using export_variable_t =
                    std::function<void(const std::string&, const std::string&, std::stringstream&)>;
                static const std::unordered_map<std::string, export_variable_t>
                    export_variable_functions = { { "bourne", export_variable_bourne },
                                                  { "csh", export_variable_csh_tcsh },
                                                  { "tcsh", export_variable_csh_tcsh } };

                fs::path home_dir = get("HOME");
                fs::path profile_path = home_dir / ("." + shell + "rc");
                std::stringstream file;

                {
                    std::ifstream stream(profile_path);
                    if (stream.is_open()) {
                        std::string line;

                        while (std::getline(stream, line)) {
                            file << line << '\n';
                        }

                        stream.close();
                    }
                }

                if (export_variable_functions.find(shell) != export_variable_functions.end()) {
                    auto func = export_variable_functions.at(shell);
                    func(key, value, file);
                } else {
                    file << "export " << key << "=" << std::quoted(value);
                }

                {
                    std::ofstream stream(profile_path);
                    if (stream.is_open()) {
                        stream << file.str() << std::flush;
                        stream.close();
                    } else {
                        return false;
                    }
                }
            }
        } else {
            return false;
        }

        return setenv(key.c_str(), value.c_str(), 1) == 0;
#endif
    }

    std::string environment::get(const std::string& key) {
        std::string value;

#ifdef SGE_PLATFORM_WINDOWS
        value = windows_getenv(key);
#else
        char* data = getenv(key.c_str());
        if (data != nullptr) {
            value = data;
        }
#endif

        return value;
    }

    bool environment::has(const std::string& key) {
#ifdef SGE_PLATFORM_WINDOWS
        return windows_hasenv(key);
#else
        return !get(key).empty();
#endif
    }

    fs::path environment::get_home_directory() {
#ifdef SGE_PLATFORM_WINDOWS
        return windows_get_home_directory();
#else
        return get("HOME");
#endif
    }
} // namespace sge
