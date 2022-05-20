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

#include "include.h"
#include "utils.h"

static bool write(const fs::path& path, const std::string& content) {
    std::string existing_content;
    if (read_file(path, &existing_content, nullptr)) {
        if (content == existing_content) {
            return true;
        }
    }

    return write_file(path, content);
}

int32_t main(int32_t argc, const char** argv) {
    if (argc < 3) {
        std::cerr << "please provide at least 2 arguments!" << std::endl;
        return EXIT_FAILURE;
    }

    fs::path dir = fs::absolute(argv[1]);
    fs::path output = fs::absolute(argv[2]);

    if (!fs::is_directory(dir)) {
        std::cerr << dir.string() << " is not a directory" << std::endl;
        return EXIT_FAILURE;
    }

    std::stringstream output_source;
    if (argc >= 4) {
        std::string license_desc = argv[3];
        std::optional<std::string> embedding = embed_license(license_desc);

        if (embedding.has_value()) {
            output_source << embedding.value();
        }
    }

    output_source << "#include <string>\n";
    output_source << "#include <unordered_map>\n";
    output_source << "#include <vector>\n\n";

    output_source << "std::unordered_map<std::string, std::vector<uint32_t>> "
                     "generated_type_face_directory = {\n";

    for (const auto& entry : fs::recursive_directory_iterator(dir)) {
        fs::path filepath = fs::absolute(entry.path());

        fs::path path = filepath.lexically_relative(dir);
        std::string embedded_path = path.string();

        size_t slash_pos;
        while ((slash_pos = embedded_path.find('\\')) != std::string::npos) {
            embedded_path.replace(slash_pos, 1, "/");
        }

        fs::path extension = path.extension();
        if (extension != ".ttf" && extension != ".otf") {
            std::cout << path.string() << " - not a type face, skipping" << std::endl;
            continue;
        }

        std::vector<uint32_t> data;
        if (!read_file(filepath, nullptr, &data)) {
            std::cerr << "could not read file: " << filepath.string() << std::endl;
            return EXIT_FAILURE;
        }

        std::string embedding = embed_binary(data);
        output_source << "\t{ " << std::quoted(embedded_path) << ", " << embedding << " },\n";
    }

    output_source << "};\n";
    std::string file_data = output_source.str();

    std::string output_path_string = output.string();
    std::cout << "writing data to source file " << output_path_string << "..." << std::endl;

    if (!write(output, file_data)) {
        std::cerr << "could not write to file: " << output_path_string << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "wrote type face directory to file: " << output_path_string << std::endl;
    return EXIT_SUCCESS;
}