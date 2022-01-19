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

namespace SGE.Components
{
    /// <summary>
    /// A transform component describes an entity's location in a scene.
    /// </summary>
    public sealed class TransformComponent
    {
        internal TransformComponent(IntPtr address)
        {
            mAddress = address;
        }
        
        public Vector2 Translation
        {
            get
            {
                Vector2 translation;
                InternalCalls.GetTranslation(mAddress, out translation);
                return translation;
            }
            set => InternalCalls.SetTranslation(mAddress, value);
        }

        public float Rotation
        {
            get => InternalCalls.GetRotation(mAddress);
            set => InternalCalls.SetRotation(mAddress, value);
        }

        public Vector2 Scale
        {
            get
            {
                Vector2 scale;
                InternalCalls.GetScale(mAddress, out scale);
                return scale;
            }
            set => InternalCalls.SetScale(mAddress, value);
        }

        private readonly IntPtr mAddress;
    }
}