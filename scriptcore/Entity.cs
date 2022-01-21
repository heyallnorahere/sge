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

using SGE.Components;
using System;

namespace SGE
{
    /// <summary>
    /// Entities are objects that give scenes behavior.
    /// </summary>
    public sealed class Entity
    {
        internal Entity(uint id, Scene scene)
        {
            mID = id;
            mScene = scene;
        }

        /// <summary>
        /// Checks if this entity has a component of the specified type.
        /// </summary>
        /// <typeparam name="T">A type of a component.</typeparam>
        /// <returns>See above.</returns>
        public bool HasComponent<T>() => InternalCalls.HasComponent(typeof(T), this);

        /// <summary>
        /// Retrieves a component of the specified type.
        /// </summary>
        /// <typeparam name="T">The type of component to search for.</typeparam>
        /// <returns>The component.</returns>
        public T GetComponent<T>() where T : Component<T>, new()
        {
            IntPtr address = InternalCalls.GetComponent(typeof(T), this);

            var component = new T();
            component.SetInternalData(address, this);
            return component;
        }

        /// <summary>
        /// The <see cref="SGE.GUID"/> of this entity.
        /// </summary>
        public GUID GUID => InternalCalls.GetGUID(this);

        /// <summary>
        /// The ID of this entity within the scene.
        /// </summary>
        public uint ID => mID;

        /// <summary>
        /// The scene that owns this entity.
        /// </summary>
        public Scene Scene => mScene;

        public override bool Equals(object obj)
        {
            if (obj is Entity entity)
            {
                return mID == entity.mID && mScene == entity.mScene;
            }
            else
            {
                return false;
            }
        }

        public override int GetHashCode()
        {
            return (mID.GetHashCode() << 1) ^ mScene.GetHashCode();
        }

        public static bool operator ==(Entity lhs, Entity rhs)
        {
            bool lhsNull = (lhs is null);
            bool rhsNull = (rhs is null);

            if (lhsNull || rhsNull)
            {
                return lhsNull && rhsNull;
            }
            else
            {
                return lhs.Equals(rhs);
            }
        }
        public static bool operator !=(Entity lhs, Entity rhs) => !(lhs == rhs);

        private readonly uint mID;
        private readonly Scene mScene;
    }
}