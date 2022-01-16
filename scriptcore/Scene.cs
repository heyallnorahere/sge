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
    /// <summary>
    /// A scene is a set of entities that can be serialized to a file.
    /// </summary>
    public sealed class Scene
    {
        internal Scene(IntPtr nativeAddress)
        {
            mNativeAddress = nativeAddress;
        }

        /// <summary>
        /// Creates an entity.
        /// </summary>
        /// <param name="name">The name to give the created entity. "Entity" if no name is passed.</param>
        /// <returns>The created entity.</returns>
        public Entity CreateEntity(string name = null)
        {
            string entityName = name ?? string.Empty;
            uint entityID = InternalCalls.CreateEntity(entityName, mNativeAddress);
            return new Entity(entityID, this);
        }

        /// <summary>
        /// Creates an entity with a specific <see cref="GUID"/>.
        /// </summary>
        /// <param name="id">The ID to create the entity with.</param>
        /// <param name="name">The name to give the created entity. "Entity" if no name is passed.</param>
        /// <returns>The created entity.</returns>
        public Entity CreateEntity(GUID id, string name = null)
        {
            string entityName = name ?? string.Empty;
            uint entityID = InternalCalls.CreateEntityWithGUID(id, entityName, mNativeAddress);
            return new Entity(entityID, this);
        }

        /// <summary>
        /// Destroys an entity.
        /// </summary>
        /// <param name="entity">The entity to destroy.</param>
        /// <exception cref="InvalidOperationException" />
        public void DestroyEntity(Entity entity)
        {
            if (entity.Scene != this)
            {
                throw new InvalidOperationException("Attempted to destroy an entity with an incompatible scene!");
            }

            InternalCalls.DestroyEntity(entity.ID, mNativeAddress);
        }

        /// <summary>
        /// Attempts to find an entity with the given <see cref="GUID"/>.
        /// </summary>
        /// <param name="id">The ID to search for.</param>
        /// <returns>An entity, if one was found. Returns null on failure.</returns>
        public Entity FindEntity(GUID id)
        {
            uint entityID;
            if (InternalCalls.FindEntity(id, out entityID, mNativeAddress))
            {
                return new Entity(entityID, this);
            }

            return null;
        }

        public override bool Equals(object obj)
        {
            if (obj is Scene scene)
            {
                return mNativeAddress == scene.mNativeAddress;
            }
            else
            {
                return false;
            }
        }

        public override int GetHashCode() => mNativeAddress.GetHashCode();

        public static bool operator ==(Scene lhs, Scene rhs)
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

        public static bool operator !=(Scene lhs, Scene rhs) => !(lhs == rhs);

        internal readonly IntPtr mNativeAddress;
    }
}