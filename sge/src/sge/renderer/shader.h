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
    enum class shader_language { glsl, hlsl };
    enum class shader_stage { vertex, fragment };

    class shader : public ref_counted {
    public:
        static ref<shader> create(const fs::path& path, shader_language language);
        static ref<shader> create(const fs::path& path);

        virtual ~shader() = default;

        virtual void reload() = 0;
        virtual const fs::path& get_path() = 0;

    protected:
        static void parse_source(const fs::path& path,
                                 std::map<shader_stage, std::string>& output_source);
    };

    class shader_library {
    public:
        shader_library() = default;

        shader_library(const shader_library&) = delete;
        shader_library& operator=(const shader_library&) = delete;

        void reload_all();

        bool add(const std::string& name, ref<shader> _shader);
        ref<shader> add(const std::string& name, const fs::path& path);
        ref<shader> add(const std::string& name, const fs::path& path, shader_language language);
        ref<shader> get(const std::string& name);

    private:
        std::unordered_map<std::string, ref<shader>> m_library;
    };
} // namespace sge