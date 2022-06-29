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
namespace sge {
    struct process_info {
        fs::path executable;
        std::string cmdline;

        fs::path workdir, output_file;
        bool detach = false;
    };

    class environment {
    public:
        environment() = delete;

        static int32_t run_command(const process_info& info);

        static bool set(const std::string& key, const std::string& value);
        static std::string get(const std::string& key);
        static bool has(const std::string& key);

        static fs::path get_home_directory();
        static uint64_t get_process_id();
    };
} // namespace sge
