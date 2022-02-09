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
#ifdef SGE_PLATFORM_WINDOWS
#include "sge/platform/windows/windows_environment.h"
#elif defined(SGE_PLATFORM_LINUX)
#define getenv secure_getenv
#endif

namespace sge {
	static void export_variable_bourne(const std::string& key, const std::string& value, std::stringstream& stream) {
		stream << key << "=" << std::quoted(value) << "\nexport " << key;
	}

	static void export_variable_csh_tcsh(const std::string& key, const std::string& value, std::stringstream& stream) {
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
				using export_variable_t = std::function<void(const std::string&, const std::string&, std::stringstream&)>;
				static const std::unordered_map<std::string, export_variable_t> export_variable_functions = {
					{ "bourne", export_variable_bourne },
					{ "csh", export_variable_csh_tcsh },
					{ "tcsh", export_variable_csh_tcsh }
				};

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
} // namespace sge
