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
    /// <summary>
    /// A GUID is a <b>G</b>lobally <b>U</b>nique <b>ID</b>entifier that can be used to identify an <see cref="Entity"/> through a <see cref="Scene"/>.
    /// </summary>
    public struct GUID
    {
        public ulong Value;
        public GUID(ulong value)
        {
            Value = value;
        }

        public static GUID Generate() => InternalCalls.GenerateGUID();
        public static implicit operator GUID(ulong id) => new GUID(id);
        public static implicit operator ulong(GUID guid) => guid.Value;
    }
}