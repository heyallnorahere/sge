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

using System.Collections.Generic;

namespace SGE.Debugger
{
    internal sealed class ObjectRegistry<T>
    {
        public ObjectRegistry()
        {
            mCurrentHandle = 0;
            mRegistry = new Dictionary<int, T>();
        }

        public void Clear()
        {
            mCurrentHandle = 0;
            mRegistry.Clear();
        }

        public int Insert(T value)
        {
            int handle = mCurrentHandle++;
            mRegistry.Add(handle, value);
            return handle;
        }

        public bool TryGet(int handle, out T value) => mRegistry.TryGetValue(handle, out value);

        public T Get(int handle) => mRegistry[handle];
        public T Get(int handle, T defaultValue)
        {
            if (TryGet(handle, out T retrievedValue))
            {
                return retrievedValue;
            }
            else
            {
                return defaultValue;
            }
        }

        private int mCurrentHandle;
        private Dictionary<int, T> mRegistry;
    }
}
