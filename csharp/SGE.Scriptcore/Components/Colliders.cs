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
    public abstract class Collider<T> : Component<T> where T : Collider<T>, new()
    {
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

        public float RestitutionThreshold
        {
            get => CoreInternalCalls.GetRestitutionThreshold(mAddress);
            set => CoreInternalCalls.SetRestitutionThreshold(mAddress, mParent, value);
        }

        public bool IsSensor
        {
            get => CoreInternalCalls.IsColliderSensor(mAddress);
            set => CoreInternalCalls.SetIsColliderSensor(mAddress, mParent, value);
        }
    }

    /// <summary>
    /// A box collider component gives its parent entity a rectangular collider.
    /// </summary>
    public sealed class BoxColliderComponent : Collider<BoxColliderComponent>
    {
        public Vector2 Size
        {
            get
            {
                Vector2 size;
                CoreInternalCalls.GetBoxSize(mAddress, out size);
                return size;
            }
            set => CoreInternalCalls.SetBoxSize(mAddress, mParent, value);
        }
    }

    public sealed class CircleColliderComponent : Collider<CircleColliderComponent>
    {
        public float Radius
        {
            get => CoreInternalCalls.GetCircleRadius(mAddress);
            set => CoreInternalCalls.SetCircleRadius(mAddress, mParent, value);
        }
    }

    public sealed class ShapeColliderComponent : Collider<ShapeColliderComponent>
    {
        public Shape Shape
        {
            get
            {
                CoreInternalCalls.GetShapeColliderPointer(mAddress, out IntPtr address);

                if (address != IntPtr.Zero)
                {
                    return new Shape(address);
                }
                else
                {
                    return null;
                }
            }
            set => CoreInternalCalls.SetShapeColliderPointer(mAddress, mParent, value?.mAddress ?? IntPtr.Zero);
        }
    }
}