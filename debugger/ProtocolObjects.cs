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

using Mono.Debugging.Client;
using Newtonsoft.Json;
using System.Collections.Generic;

namespace SGE.Debugger
{
    internal sealed class DebuggerSource
    {
        public DebuggerSource(string name, string path, int sourceReference, string hint)
        {
            Name = name;
            Path = path;
            SourceReference = sourceReference;
            PresentationHint = hint;
        }

        [JsonProperty(PropertyName = "name")]
        public string Name { get; }

        [JsonProperty(PropertyName = "path")]
        public string Path { get; }

        [JsonProperty(PropertyName = "sourceReference")]
        public int SourceReference { get; }

        [JsonProperty(PropertyName = "presentationHint")]
        public string PresentationHint { get; }
    }

    internal sealed class DebuggerStackFrame
    {
        public DebuggerStackFrame(int id, string name, DebuggerSource source, int line, int column, string hint)
        {
            ID = id;
            Name = name;
            Source = source;
            Line = line;
            Column = column;
            Hint = hint;
        }

        [JsonProperty(PropertyName = "id")]
        public int ID { get; }

        [JsonProperty(PropertyName = "name")]
        public string Name { get; }

        [JsonProperty(PropertyName = "source")]
        public DebuggerSource Source { get; }

        [JsonProperty(PropertyName = "line")]
        public int Line { get; }

        [JsonProperty(PropertyName = "column")]
        public int Column { get; }

        [JsonProperty(PropertyName = "hint")]
        public string Hint { get; }
    }

    internal sealed class DebuggerThread
    {
        public DebuggerThread(ThreadInfo info)
        {
            Thread = info;
        }

        [JsonProperty(PropertyName = "id")]
        public long ID => Thread.Id;

        [JsonProperty(PropertyName = "name")]
        public string Name => Thread.Name;

        [JsonIgnore]
        public ThreadInfo Thread { get; }
    }

    internal sealed class DebuggerVariable
    {
        public static DebuggerVariable FromObjectValue(ObjectValue value,
                                                       ObjectRegistry<IEnumerable<ObjectValue>> registry)
        {
            if (value == null)
            {
                return null;
            }

            string displayValue = value.DisplayValue;
            if (displayValue.Length > 1 &&
                displayValue[0] == '{' &&
                displayValue[displayValue.Length - 1] == '}')
            {
                displayValue = displayValue.Substring(1, displayValue.Length - 2);
            }

            int handle = 0;
            if (value.HasChildren)
            {
                var children = value.GetAllChildren();
                handle = registry.Insert(children);
            }

            return new DebuggerVariable(value.Name, displayValue, value.TypeName, handle);
        }

        public DebuggerVariable(string name, string value, string type, int childrenSetId)
        {
            Name = name;
            Value = value;
            Type = type;
            Children = childrenSetId;
        }

        [JsonProperty(PropertyName = "name")]
        public string Name { get; }

        [JsonProperty(PropertyName = "value")]
        public string Value { get; }

        [JsonProperty(PropertyName = "type")]
        public string Type { get; }

        [JsonProperty(PropertyName = "childrenSetId")]
        public int Children { get; }
    }

    internal sealed class DebuggerScope
    {
        public DebuggerScope(string name, int variableSetId, bool expensive)
        {
            Name = name;
            VariableSetID = variableSetId;
            Expensive = expensive;
        }

        [JsonProperty(PropertyName = "name")]
        public string Name { get; }

        [JsonProperty(PropertyName = "variableSetId")]
        public int VariableSetID { get; }

        [JsonProperty(PropertyName = "expensive")]
        public bool Expensive { get; }
    }
}