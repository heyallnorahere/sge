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
    public enum ImageFormat : int
    {
        RGB8_UNORM = 0,
        RGB8_SRGB,

        RGBA8_UNORM,
        RGBA8_SRGB
    }

    [Flags]
    public enum ImageUsage : uint
    {
        None = 0x0,
        Texture = 0x1,
        Attachment = 0x2,
        Storage = 0x4,
        Transfer = 0x8
    }

    public sealed class Image2D
    {
        internal Image2D(IntPtr address, bool addRef)
        {
            mAddress = address;
            if (addRef)
            {
                CoreInternalCalls.AddRef_image_2d(address);
            }
        }

        ~Image2D() => CoreInternalCalls.RemoveRef_image_2d(mAddress);

        public int Width => CoreInternalCalls.GetImageWidth(mAddress);
        public int Height => CoreInternalCalls.GetImageHeight(mAddress);
        public int MipLevelCount => CoreInternalCalls.GetImageMipLevelCount(mAddress);
        public int ArrayLayerCount => CoreInternalCalls.GetImageArrayLayerCount(mAddress);
        public ImageFormat Format => CoreInternalCalls.GetImageFormat(mAddress);
        public ImageUsage Usage => CoreInternalCalls.GetImageUsage(mAddress);

        private readonly IntPtr mAddress;
    }
}