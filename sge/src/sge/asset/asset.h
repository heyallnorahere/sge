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
#include "sge/core/guid.h"
namespace sge {
    enum class asset_type : int32_t {
        shader = 0,
        texture_2d,
        prefab,
        sound,
        shape
    };

    class asset : public ref_counted {
    public:
        guid id;

        virtual ~asset() = default;

        virtual asset_type get_asset_type() = 0;
        virtual const fs::path& get_path() = 0;

        virtual bool reload() = 0;
    };

    struct asset_desc {
        std::optional<guid> id;
        fs::path path;
        std::optional<asset_type> type;
    };
} // namespace sge