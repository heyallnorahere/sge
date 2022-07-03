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
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;

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
    }
}
