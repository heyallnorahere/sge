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
            get => CoreInternalCalls.GetBodyType(mAddress);
            set => CoreInternalCalls.SetBodyType(mAddress, mParent, value);
        }

        /// <summary>
        /// If the rotation of this object cannot be changed through the physics world.
        /// </summary>
        public bool FixedRotation
        {
            get => CoreInternalCalls.GetFixedRotation(mAddress);
            set => CoreInternalCalls.SetFixedRotation(mAddress, mParent, value);
        }

        /// <summary>
        /// The velocity of this object.
        /// </summary>
        public Vector2 Velocity
        {
            get
            {
                if (!CoreInternalCalls.GetVelocity(mParent, out Vector2 velocity))
                {
                    velocity = new Vector2(0f);
                }

                return velocity;
            }
            set => CoreInternalCalls.SetVelocity(mParent, value);
        }

        /// <summary>
        /// The angular velocity of this object.
        /// </summary>
        public float AngularVelocity
        {
            get
            {
                if (!CoreInternalCalls.GetAngularVelocity(mParent, out float velocity))
                {
                    velocity = 0f;
                }

                return velocity;
            }
            set => CoreInternalCalls.SetAngularVelocity(mParent, value);
        }

        /// <summary>
        /// Applies a force to this rigid body.
        /// </summary>
        /// <param name="force">The force to apply.</param>
        /// <param name="point">The point on this rigid body at which to apply this force.</param>
        /// <param name="wake">Whether to wake the object. Default is true.</param>
        public void ApplyForce(Vector2 force, Vector2 point, bool wake = true)
        {
            CoreInternalCalls.ApplyForce(mParent, force, point, wake);
        }

        /// <summary>
        /// Applies a force to the center of this rigid body.
        /// </summary>
        /// <param name="force">The force to apply.</param>
        /// <param name="wake">Whether to wake the object. Default is true.</param>
        public void ApplyForce(Vector2 force, bool wake = true)
        {
            CoreInternalCalls.ApplyForceToCenter(mParent, force, wake);
        }

        /// <summary>
        /// Applies a force to this rigid body without adding torque.
        /// </summary>
        /// <param name="impulse">The impulse to apply.</param>
        /// <param name="point">The point on this rigid body at which to apply this force.</param>
        /// <param name="wake">Whether to wake the object. Default is true.</param>
        public void ApplyLinearImpulse(Vector2 impulse, Vector2 point, bool wake = true)
        {
            CoreInternalCalls.ApplyLinearImpulse(mParent, impulse, point, wake);
        }

        /// <summary>
        /// Applies a force to the center of this rigid body without adding torque.
        /// </summary>
        /// <param name="impulse">The impulse to apply.</param>
        /// <param name="wake">Whether to wake the object. Default is true.</param>
        public void ApplyLinearImpulse(Vector2 impulse, bool wake = true)
        {
            CoreInternalCalls.ApplyLinearImpulseToCenter(mParent, impulse, wake);
        }

        /// <summary>
        /// Applies torque to the angular velocity of this rigid body.
        /// </summary>
        /// <param name="torque">The amount of torque to add.</param>
        /// <param name="wake">Whether to wake the object. Default is true.</param>
        public void ApplyTorque(float torque, bool wake = true)
        {
            CoreInternalCalls.ApplyTorque(mParent, torque, wake);
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
            get => CoreInternalCalls.GetFilterCategory(mComponent.Address);
            set => CoreInternalCalls.SetFilterCategory(mComponent.Address, value);
        }

        /// <summary>
        /// Specifies what categories this object can collide with.
        /// </summary>
        public ushort MaskBits
        {
            get => CoreInternalCalls.GetFilterMask(mComponent.Address);
            set => CoreInternalCalls.SetFilterMask(mComponent.Address, value);
        }

        private readonly RigidBodyComponent mComponent;
    }
}