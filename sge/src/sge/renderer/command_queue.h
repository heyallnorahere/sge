/*
   Copyright 2021 Nora Beda

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
#include "sge/renderer/command_list.h"
namespace sge {
    enum class command_list_type { graphics, compute, transfer };
    class command_queue : public ref_counted {
    public:
        static ref<command_queue> create(command_list_type type);

        virtual ~command_queue() = default;

        virtual command_list& get() = 0;
        virtual void submit(command_list& cmdlist, bool wait = false) = 0;

        virtual command_list_type get_type() = 0;
    };
} // namespace sge