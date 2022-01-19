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
    public enum BodyType
    {
        Static = 0,
        Kinematic,
        Dynamic
    }

    /// <summary>
    /// A rigid body component describes how other entities should affect its parent.
    /// </summary>
    public sealed class RigidBodyComponent
    {
        internal RigidBodyComponent(IntPtr address)
        {
            mAddress = address;
        }

        public BodyType BodyType
        {
            get => InternalCalls.GetBodyType(mAddress);
            set => InternalCalls.SetBodyType(mAddress, value);
        }

        public bool FixedRotation
        {
            get => InternalCalls.GetFixedRotation(mAddress);
            set => InternalCalls.SetFixedRotation(mAddress, value);
        }

        private readonly IntPtr mAddress;
    }
}