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
    public sealed class SoundController
    {
        internal SoundController(IntPtr address)
        {
            mAddress = address;
        }

        ~SoundController() => CoreInternalCalls.DeleteSoundControllerPointer(mAddress);

        internal readonly IntPtr mAddress;
    }

    [TypedAsset(AssetType.Sound)]
    public sealed class Sound : Asset
    {
        internal Sound(IntPtr address) : base(address)
        {
            CoreInternalCalls.AddRef_sound(address);
        }

        ~Sound() => CoreInternalCalls.RemoveRef_sound(mAddress);

        public static SoundController Play(Sound sound, bool repeat = false)
        {
            CoreInternalCalls.PlaySound(sound.mAddress, repeat, out IntPtr controller);
            return new SoundController(controller);
        }

        public static bool Stop(SoundController controller) => CoreInternalCalls.StopSound(controller.mAddress);

        public float Duration => CoreInternalCalls.GetSoundDuration(mAddress);
    }
}
