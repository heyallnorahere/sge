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
using System.IO;

namespace SGE
{
    public enum TextureWrap
    {
        Clamp = 0,
        Repeat
    }

    public enum TextureFilter
    {
        Linear = 0,
        Nearest
    }

    [TypedAsset(AssetType.Texture2D)]
    public sealed class Texture2D : Asset
    {
        private static IntPtr Load(string path)
        {
            CoreInternalCalls.LoadTexture2D(path, out IntPtr address);
            if (address == IntPtr.Zero)
            {
                throw new FileNotFoundException();
            }

            return address;
        }

        public Texture2D(string path) : base(Load(path)) { }
        internal Texture2D(IntPtr address) : base(address)
        {
            CoreInternalCalls.AddRef_texture_2d(mAddress);
        }

        ~Texture2D() => CoreInternalCalls.RemoveRef_texture_2d(mAddress);

        public TextureWrap Wrap => CoreInternalCalls.GetWrapTexture2D(mAddress);
        public TextureFilter Filter => CoreInternalCalls.GetFilterTexture2D(mAddress);
    }
}