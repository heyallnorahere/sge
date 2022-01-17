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
#include "sge/renderer/shader.h"
namespace sge {
    class directx_shader : public shader {
    public:
        directx_shader(const fs::path& path, shader_language language);
        virtual ~directx_shader() override = default;

        virtual void reload() override;
        virtual const fs::path& get_path() override { return m_path; }

        const std::map<shader_stage, ComPtr<ID3D10Blob>>& get_dxil() { return m_dxil; }

    private:
        void load();

        fs::path m_path;
        shader_language m_language;

        std::map<shader_stage, ComPtr<ID3D10Blob>> m_dxil;
    };
} // namespace sge
