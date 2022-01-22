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
#include "sge/asset/asset.h"
namespace sge {
    class asset_serializer {
    public:
        static void init();
        static bool serialize(ref<asset> _asset);
        static bool deserialize(const asset_desc& desc, ref<asset>& _asset);

    protected:
        virtual bool serialize_impl(ref<asset> _asset) = 0;
        virtual bool deserialize_impl(const fs::path& path, ref<asset>& _asset) = 0;
    };

    class shader_serializer : public asset_serializer {
    protected:
        virtual bool serialize_impl(ref<asset> _asset) override { return true; }
        virtual bool deserialize_impl(const fs::path& path, ref<asset>& _asset) override;
    };

    class texture2d_serializer : public asset_serializer {
    protected:
        virtual bool serialize_impl(ref<asset> _asset) override;
        virtual bool deserialize_impl(const fs::path& path, ref<asset>& _asset) override;
    };
} // namespace sge