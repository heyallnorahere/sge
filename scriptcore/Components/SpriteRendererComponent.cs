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
    /// A sprite renderer component, as the name suggests, renders a sprite.
    /// </summary>
    public sealed class SpriteRendererComponent
    {
        internal SpriteRendererComponent(IntPtr address)
        {
            mAddress = address;
            throw new NotImplementedException();
        }

        // todo(nora): we need a vector4 struct and a managed texture class

        private readonly IntPtr mAddress;
    }
}