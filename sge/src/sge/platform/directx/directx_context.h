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
    struct dx_data;
    class directx_context {
    public:
        static void create(D3D_FEATURE_LEVEL feature_level);
        static void destroy();
        static directx_context& get();

        directx_context(const directx_context&) = delete;
        directx_context& operator=(const directx_context&) = delete;

    private:
        directx_context() = default;

        void init(D3D_FEATURE_LEVEL feature_level);
        void shutdown();

        dx_data* m_data;
    };
} // namespace sge
