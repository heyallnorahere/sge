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
#include "sge/renderer/renderer.h"
namespace sge {
    class directx_renderer : public renderer_api {
    public:
        virtual void init() override;
        virtual void shutdown() override;
        virtual void wait() override;

        virtual void submit(const draw_data& data) override;

        virtual device_info query_device_info() override;
    };
} // namespace sge
