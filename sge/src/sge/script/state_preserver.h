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
    class state_preserver {
    public:
        state_preserver(const std::vector<void*>& reloaded_assemblies);
        ~state_preserver();

        state_preserver(const state_preserver&) = delete;
        state_preserver& operator=(const state_preserver&) = delete;

    private:
        struct managed_object_data_t {
            bool field, value_type;
            void* identifier;
            void* data;
            uint32_t handle;
        };

        struct managed_object_t {
            std::string _class;
            void* assembly;
            void* object = nullptr;

            std::unordered_map<uint32_t, std::vector<uint32_t*>> refs;
            std::unordered_map<std::string, managed_object_data_t> data;
        };

        void retrieve_data();
        bool is_tracked_class(void* _class);
        void parse_object(void* object_ptr);
        void parse_object_data(void* value, void* type, managed_object_data_t& data);
        void destroy_objects();

        bool restore_object(void* object_ptr);
        void restore_objects();
        void restore_refs();

        std::vector<void*> m_reloaded_assemblies;
        std::unordered_map<void*, managed_object_t> m_data;
    };
} // namespace sge
