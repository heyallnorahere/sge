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

using System;

namespace SGE
{
    public enum ShaderLanguage
    {
        GLSL = 0,
        HLSL
    }

    [TypedAsset(AssetType.Shader)]
    public sealed class Shader : Asset
    {
        private static IntPtr Load(string path)
        {
            InternalCalls.LoadShaderAuto(path, out IntPtr address);
            if (address == IntPtr.Zero)
            {
                throw new Exception("Failed to load shader!");
            }

            return address;
        }

        private static IntPtr Load(string path, ShaderLanguage language)
        {
            InternalCalls.LoadShaderExplicit(path, language, out IntPtr address);
            if (address == IntPtr.Zero)
            {
                throw new Exception("Failed to load shader!");
            }

            return address;
        }

        public Shader(string path) : base(Load(path)) { }
        public Shader(string path, ShaderLanguage language) : base(Load(path, language)) { }

        internal Shader(IntPtr address) : base(address)
        {
            InternalCalls.AddRef_shader(address);
        }
        ~Shader() => InternalCalls.RemoveRef_shader(mAddress);
    }
}