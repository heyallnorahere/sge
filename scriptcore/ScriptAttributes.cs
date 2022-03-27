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

namespace SGE
{
    /// <summary>
    /// A property with this attribute applied will not be serialized or shown in an editor.
    /// </summary>
    [AttributeUsage(AttributeTargets.Property, AllowMultiple = false)]
    public sealed class UnserializedAttribute : Attribute
    {
        // this should really just act as a flag
    }

    /// <summary>
    /// Indicates to the editor that the property should be under the specific header.
    /// If no header is specified, no header will contain this property.
    /// </summary>
    [AttributeUsage(AttributeTargets.Property, AllowMultiple = false)]
    public sealed class SectionHeaderAttribute : Attribute
    {
        public SectionHeaderAttribute(string path)
        {
            var names = new List<string>();
            var segments = path.Split('/', '\\');

            foreach (string segment in segments)
            {
                if (segment.Length == 0 || segment == ".")
                {
                    continue;
                }

                if (segment == "..")
                {
                    names.RemoveAt(names.Count - 1);
                }
                else
                {
                    names.Add(segment);
                }
            }

            if (names.Count == 0)
            {
                names.Add(string.Empty);
            }

            HeaderNames = names;
        }

        public IReadOnlyList<string> HeaderNames { get; }
    }
}