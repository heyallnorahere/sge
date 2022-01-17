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
#include "sge/renderer/shader_internals.h"
namespace sge {
    struct included_file_info {
        std::string content, path;
    };

    shaderc_include_result* shaderc_file_finder::GetInclude(const char* requested_source,
                                                            shaderc_include_type type,
                                                            const char* requesting_source,
                                                            size_t include_depth) {
        // get c++17 paths
        fs::path requested_path = requested_source;
        fs::path requesting_path = requesting_source;

        // sort out paths
        if (!requesting_path.has_parent_path()) {
            requesting_path = fs::absolute(requesting_path);
        }
        if (type != shaderc_include_type_standard) {
            requested_path = requesting_path.parent_path() / requested_path;
        }

        // read data
        auto file_info = new included_file_info;
        file_info->path = requested_path.string();
        {
            if (!fs::exists(requested_path)) {
                throw std::runtime_error(requested_path.string() + " does not exist!");
            }

            std::ifstream file(requested_path);
            std::string line;
            while (std::getline(file, line)) {
                file_info->content += line + '\n';
            }
            file.close();
        }

        // return result
        auto result = new shaderc_include_result;
        result->user_data = file_info;
        result->content = file_info->content.c_str();
        result->content_length = file_info->content.length();
        result->source_name = file_info->path.c_str();
        result->source_name_length = file_info->path.length();
        return result;
    }

    void shaderc_file_finder::ReleaseInclude(shaderc_include_result* data) {
        delete (included_file_info*)data->user_data;
        delete data;
    }
} // namespace sge
