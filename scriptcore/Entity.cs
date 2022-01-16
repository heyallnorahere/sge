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

namespace SGE
{
    public sealed class Entity
    {
        internal Entity(uint id, Scene scene)
        {
            mID = id;
            mScene = scene;
        }

        public bool HasComponent<T>() => InternalCalls.HasComponent(typeof(T), mID, mScene.mNativeAddress);
        public T GetComponent<T>() => (T)InternalCalls.GetComponent(typeof(T), mID, mScene.mNativeAddress);

        public GUID GUID => InternalCalls.GetGUID(mID, mScene.mNativeAddress);
        public uint ID => mID;
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