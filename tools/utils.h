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
inline bool read_file(const fs::path& path, std::string* text,
                      std::vector<uint32_t>* data = nullptr) {
    bool binary = (data != nullptr);
    if (text != nullptr || binary) {
        if (text != nullptr && binary) {
            std::cout << "both text and binary requested - using text" << std::endl;
            binary = false;
        }

        std::ios::openmode mode = std::ios::in;
        if (binary) {
            mode |= std::ios::binary;
        }

        std::ifstream file(path, mode);
        std::optional<std::string> result;

        if (file.is_open()) {
            if (binary) {
                auto file_data = std::vector<char>(std::istreambuf_iterator<char>(file),
                                                   std::istreambuf_iterator<char>());

                *data = std::vector<uint32_t>(file_data.size() / 4);
                std::copy(file_data.begin(), file_data.end(), (char*)data->data());
            } else {
                std::stringstream stream;
                std::string line;

                while (std::getline(file, line)) {
                    stream << line << '\n';
                }

                *text = stream.str();
            }

            file.close();
            return true;
        }
    }

    return false;
}

inline bool write_file(const fs::path& path, const std::string& text) {
    std::ofstream file(path, std::ios::out);
    if (!file.is_open()) {
        return false;
    }

    file << text;
    file.close();

    return true;
}

inline bool write_file(const fs::path& path, const std::vector<uint32_t>& data) {
    std::ofstream file(path, std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    std::copy(data.begin(), data.end(), std::ostreambuf_iterator<char>(file));
    file.close();

    return true;
}

inline std::optional<std::string> embed_license(const std::string& desc) {
    std::optional<std::string> result;

    size_t colon_pos = desc.find(':');
    fs::path license_path = fs::absolute(desc.substr(0, colon_pos));

    std::string license;
    if (read_file(license_path, &license)) {
        size_t current_offset = 0;
        bool section_found = true;

        size_t section_start = 0;
        size_t section_end = 0;
        if (colon_pos != std::string::npos) {
            std::string range = desc.substr(colon_pos + 1);

            bool valid_range = !range.empty();
            if (valid_range) {
                size_t hyphon_pos = range.find('-');

                std::string start_pos = range.substr(0, hyphon_pos);
                if (start_pos.empty()) {
                    std::cout << "no start pos specified!" << std::endl;
                    valid_range = false;
                }

                if (valid_range) {
                    int32_t start_line = atoi(start_pos.c_str());
                    if (start_line <= 0) {
                        std::cout << "the start line must be greater than 0!" << std::endl;
                        valid_range = false;
                    }

                    if (valid_range) {
                        section_start = (size_t)start_line;

                        if (hyphon_pos != std::string::npos) {
                            std::string end_pos = range.substr(hyphon_pos + 1);
                            if (!end_pos.empty()) {
                                int32_t end_line = atoi(end_pos.c_str());
                                if (end_line > start_line) {
                                    section_end = (size_t)end_line;
                                }
                            }
                        }

                        for (int32_t i = 0; i < start_line - 1; i++) {
                            size_t newline_pos = license.find('\n', current_offset);
                            if (newline_pos == std::string::npos) {
                                section_found = false;
                                break;
                            }

                            current_offset = newline_pos + 1;
                        }
                    }
                }
            }

            if (!valid_range) {
                std::cout << "invalid range - embedding entire license" << std::endl;
            }
        }

        if (section_found) {
            size_t end = std::string::npos;

            if (section_end > 0) {
                size_t line_count = section_end - section_start;

                for (size_t i = 0; i < line_count; i++) {
                    size_t offset = current_offset;
                    if (end != std::string::npos) {
                        offset = end + 1;
                    } else if (i > 0) {
                        break;
                    }

                    end = license.find('\n', offset);
                }

                if (end != std::string::npos) {
                    end -= current_offset;
                }
            }

            std::string embedded_section = license.substr(current_offset, end);
            if (*embedded_section.rbegin() != '\n') {
                embedded_section += '\n';
            }

            result = "/*\n" + embedded_section + "*/\n\n";
        }
    }

    if (!result.has_value()) {
        std::cout << license_path.string() << " - failed to embed license" << std::endl;
    }

    return result;
}

inline std::string embed_binary(const std::vector<uint32_t>& data) {
    if (data.empty()) {
        return "{}";
    }

    std::stringstream output;
    output << "{ ";

    for (uint32_t element : data) {
        output << "0x" << std::hex << element << ", ";
    }

    output << "}";
    return output.str();
}