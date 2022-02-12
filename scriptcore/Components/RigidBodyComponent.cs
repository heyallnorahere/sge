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
    public enum BodyType
    {
        Static = 0,
        Kinematic,
        Dynamic
    }

    /// <summary>
    /// A rigid body component describes how other entities should affect its parent.
    /// </summary>
    public sealed class RigidBodyComponent : Component<RigidBodyComponent>
    {
        public BodyType BodyType
        {
            get => InternalCalls.GetBodyType(mAddress);
            set => InternalCalls.SetBodyType(mAddress, mParent, value);
        }

        public bool FixedRotation
        {
            get => InternalCalls.GetFixedRotation(mAddress);
            set => InternalCalls.SetFixedRotation(mAddress, mParent, value);
        }

        public float AngularVelocity
        {
            get
            {
                float velocity = 0f;
                InternalCalls.GetAngularVelocity(mParent, out velocity);
                return velocity;
            }
            set => InternalCalls.SetAngularVelocity(mParent, value);
        }

        public void AddForce(Vector2 force, bool wake = true)
        {
            InternalCalls.AddForce(mParent, force, wake);
        }
    }
}