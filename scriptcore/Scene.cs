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
    public sealed class Scene
    {
        internal Scene(IntPtr nativeAddress)
        {
            mNativeAddress = nativeAddress;
        }

        public Entity CreateEntity(string name = null)
        {
            string entityName = name ?? string.Empty;
            uint entityID = InternalCalls.CreateEntity(entityName, mNativeAddress);
            return new Entity(entityID, this);
        }

        public Entity CreateEntity(GUID id, string name = null)
        {
            string entityName = name ?? string.Empty;
            uint entityID = InternalCalls.CreateEntityWithGUID(id, entityName, mNativeAddress);
            return new Entity(entityID, this);
        }

        public void DestroyEntity(Entity entity)
        {
            if (entity.Scene != this)
            {
                throw new InvalidOperationException("Attempted to destroy an entity with an incompatible scene!");
            }

            InternalCalls.DestroyEntity(entity.ID, mNativeAddress);
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
            return lhs.Equals(rhs);
        }

        public static bool operator !=(Scene lhs, Scene rhs)
        {
            return !lhs.Equals(rhs);
        }

        internal readonly IntPtr mNativeAddress;
    }
}