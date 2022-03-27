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
using System.Reflection;

namespace SGE
{
    public static class Helpers
    {
        static Helpers()
        {
            mEventTypes = new Dictionary<EventID, Type>();
            foreach (Assembly assembly in AppDomain.CurrentDomain.GetAssemblies())
            {
                foreach (Type type in assembly.GetTypes())
                {
                    var attribute = type.GetCustomAttribute<EventIDAttribute>();
                    if (attribute != null && type.Extends(typeof(Event)))
                    {
                        if (mEventTypes.ContainsKey(attribute.ID))
                        {
                            continue;
                        }

                        mEventTypes.Add(attribute.ID, type);
                    }
                }
            }
        }

        public static bool Extends(this Type derived, Type baseType)
        {
            Type currentBaseType = derived.BaseType;
            if (currentBaseType == baseType)
            {
                return true;
            }
            else if (currentBaseType != null)
            {
                return currentBaseType.Extends(baseType);
            }
            else
            {
                return false;
            }
        }

        internal static bool PropertyHasAttribute(PropertyInfo property, Type attributeType)
        {
            Attribute attribute = property.GetCustomAttribute(attributeType);
            return attribute != null;
        }

        internal static Attribute GetPropertyAttribute(PropertyInfo property, Type attributeType)
        {
            return property.GetCustomAttribute(attributeType);
        }

        internal static Event CreateEvent(IntPtr address, EventID id)
        {
            if (!mEventTypes.ContainsKey(id))
            {
                Log.Error("Couldn't find the object type for an event of type: {0}", id);
                return null;
            }

            Type type = mEventTypes[id];
            ConstructorInfo constructor = type.GetConstructor(Array.Empty<Type>());
            if (constructor == null)
            {
                Log.Error("Couldn't find the constructor for type: {0}", type.FullName);
                return null;
            }

            var @event = (Event)constructor.Invoke(null);
            @event.mAddress = address;
            return @event;
        }

        private static readonly Dictionary<EventID, Type> mEventTypes;
    }
}