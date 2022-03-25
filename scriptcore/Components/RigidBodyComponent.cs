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
        /// <summary>
        /// The type of this body.
        /// </summary>
        public BodyType BodyType
        {
            get => InternalCalls.GetBodyType(mAddress);
            set => InternalCalls.SetBodyType(mAddress, mParent, value);
        }

        /// <summary>
        /// If the rotation of this object cannot be changed through the physics world.
        /// </summary>
        public bool FixedRotation
        {
            get => InternalCalls.GetFixedRotation(mAddress);
            set => InternalCalls.SetFixedRotation(mAddress, mParent, value);
        }

        /// <summary>
        /// The angular velocity of this object.
        /// </summary>
        public float AngularVelocity
        {
            get
            {
                InternalCalls.GetAngularVelocity(mParent, out float velocity);
                return velocity;
            }
            set => InternalCalls.SetAngularVelocity(mParent, value);
        }

        /// <summary>
        /// Adds a force to this rigid body.
        /// </summary>
        /// <param name="force">The force to add.</param>
        /// <param name="wake">Whether to wake the object. Default is true.</param>
        public void AddForce(Vector2 force, bool wake = true)
        {
            InternalCalls.AddForce(mParent, force, wake);
        }
    }

    /// <summary>
    /// Allows the modification of collision filters on a <see cref="RigidBodyComponent"/>.
    /// </summary>
    public sealed class CollisionFilter
    {
        public CollisionFilter(RigidBodyComponent component)
        {
            mComponent = component;
        }

        /// <summary>
        /// Specifies what category this object belongs in.
        /// </summary>
        public ushort CategoryBits
        {
            get => InternalCalls.GetFilterCategory(mComponent.Address);
            set => InternalCalls.SetFilterCategory(mComponent.Address, value);
        }

        /// <summary>
        /// Specifies what categories this object can collide with.
        /// </summary>
        public ushort MaskBits
        {
            get => InternalCalls.GetFilterMask(mComponent.Address);
            set => InternalCalls.SetFilterMask(mComponent.Address, value);
        }

        private readonly RigidBodyComponent mComponent;
    }
}