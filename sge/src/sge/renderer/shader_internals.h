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
#include <shaderc/shaderc.hpp>
namespace sge {
    class shaderc_file_finder : public shaderc::CompileOptions::IncluderInterface {
    public:
        virtual shaderc_include_result* GetInclude(const char* requested_source,
                                                   shaderc_include_type type,
                                                   const char* requesting_source,
                                                   size_t include_depth) override;

        virtual void ReleaseInclude(shaderc_include_result* data) override;
    };
} // namespace sge
