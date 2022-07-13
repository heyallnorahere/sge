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

namespace SGE.Components
{
    /// <summary>
    /// A box collider component gives its parent entity a rectangular collider.
    /// </summary>
    public sealed class BoxColliderComponent : Component<BoxColliderComponent>
    {
        public Vector2 Size
        {
            get
            {
                Vector2 size;
                CoreInternalCalls.GetSize(mAddress, out size);
                return size;
            }
            set => CoreInternalCalls.SetSize(mAddress, mParent, value);
        }

        public float Density
        {
            get => CoreInternalCalls.GetDensity(mAddress);
            set => CoreInternalCalls.SetDensity(mAddress, mParent, value);
        }

        public float Friction
        {
            get => CoreInternalCalls.GetFriction(mAddress);
            set => CoreInternalCalls.SetFriction(mAddress, mParent, value);
        }

        public float Restitution
        {
            get => CoreInternalCalls.GetRestitution(mAddress);
            set => CoreInternalCalls.SetRestitution(mAddress, mParent, value);
        }

        public float RestitutionThreashold
        {
            get => CoreInternalCalls.GetRestitutionThreashold(mAddress);
            set => CoreInternalCalls.SetRestitutionThreashold(mAddress, mParent, value);
        }
    }
}