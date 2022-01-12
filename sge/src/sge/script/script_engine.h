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
    class script_engine {
    public:
        script_engine() = delete;

        static void init();
        static void shutdown();

        static size_t load_assembly(const fs::path& path);
        static void close_assembly(size_t index);

        static void* get_class(size_t assembly, const std::string& name);
        static void* get_class(size_t assembly, const std::string& namespace_,
                               const std::string& name);
        static void* get_class(void* parameter_type);
        static void* get_parameter_type(void* _class);

        static void* alloc_object(void* _class);
        static void init_object(void* object);

        static void* get_method(void* _class, const std::string& name);
        static void* call_method(void* object, void* method, void** arguments = nullptr);

        template <typename... Args>
        static void* call_method(void* object, void* method, Args*... args) {
            std::vector<void*> args_vector = { std::forward<Args*>(args)... };
            void** arguments = args_vector.empty() ? nullptr : args_vector.data();

            return call_method(object, method, arguments);
        }
    };
} // namespace sge