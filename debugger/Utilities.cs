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

using Newtonsoft.Json.Serialization;
using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Reflection;
using System.Text;

namespace SGE.Debugger
{
    internal static class Utilities
    {
        public static IPAddress ResolveIP(string ip)
        {
            try
            {
                if (IPAddress.TryParse(ip, out IPAddress parsedAddress))
                {
                    return parsedAddress;
                }

                var entry = Dns.GetHostEntry(ip);
                if (entry.AddressList.Length > 0)
                {
                    foreach (var address in entry.AddressList)
                    {
                        if (address.AddressFamily == AddressFamily.InterNetwork)
                        {
                            return address;
                        }
                    }

                    return entry.AddressList[0];
                }
            }
            catch (Exception)
            {
                // falls through to null return
            }

            return null;
        }

        public static bool ContainsValue<K, V>(this IReadOnlyDictionary<K, V> dict, V value)
        {
            foreach (V entry in dict.Values)
            {
                if (entry.Equals(value))
                {
                    return true;
                }
            }

            return false;
        }

        public static string GetFullName(this MethodInfo method)
        {
            var methodType = method.DeclaringType;
            return $"{methodType.FullName.Replace('+', '.')}.{method.Name}";
        }
    }

    public sealed class CamelCase : NamingStrategy
    {
        public CamelCase()
        {
            ProcessDictionaryKeys = true;
            ProcessExtensionDataNames = true;
        }

        protected override string ResolvePropertyName(string name)
        {
            if (name.Length == 0)
            {
                return name;
            }

            return char.ToUpper(name[0]) + name.Substring(1);
        }

        public override string GetPropertyName(string name, bool hasSpecifiedName)
        {
            if (hasSpecifiedName)
            {
                return name;
            }

            return ToCamelCase(name);
        }

        public override string GetDictionaryKey(string key) => ToCamelCase(key);
        public override string GetExtensionDataName(string name) => ToCamelCase(name);

        private static string ToCamelCase(string name)
        {
            if (name.Length == 0)
            {
                return name;
            }

            return char.ToLower(name[0]) + name.Substring(1);
        }
    }

    internal sealed class ByteBuffer
    {
        public ByteBuffer()
        {
            mData = new List<byte>();
        }

        public void Append(byte data) => mData.Add(data);
        public void Append(IEnumerable<byte> data) => mData.AddRange(data);

        public void Append(byte[] data, int count)
        {
            for (int i = 0; i < count; i++)
            {
                mData.Add(data[i]);
            }
        }

        public Action Clear => mData.Clear;
        public void Remove(int start, int end)
        {
            if (start < 0 || start >= end || end > mData.Count)
            {
                throw new IndexOutOfRangeException();
            }

            var data = mData.ToArray();
            mData.Clear();

            for (int i = 0; i < start; i++)
            {
                mData.Add(data[i]);
            }

            for (int i = end; i < mData.Count; i++)
            {
                mData.Add(data[i]);
            }
        }

        public string GetData(Encoding encoding) => encoding.GetString(mData.ToArray());
        private List<byte> mData;
    }
}
