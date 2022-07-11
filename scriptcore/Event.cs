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
using System.Reflection;

namespace SGE
{
    public enum EventID : int
    {
        None = 0,
        WindowClose,
        WindowResize,
        KeyPressed,
        KeyReleased,
        KeyTyped,
        MouseMoved,
        MouseScrolled,
        MouseButton,
        FileChanged
    }

    public abstract class Event
    {
        public bool Handled
        {
            get => CoreInternalCalls.IsEventHandled(mAddress);
            set => CoreInternalCalls.SetEventHandled(mAddress, value);
        }

        public EventID ID
        {
            get
            {
                Type type = GetType();
                var attribute = type.GetCustomAttribute<EventIDAttribute>();

                if (attribute != null)
                {
                    return attribute.ID;
                }
                else
                {
                    return EventID.None;
                }
            }
        }

        public string Name
        {
            get
            {
                EventID id = ID;
                return id == EventID.None ? "Event" : id.ToString();
            }
        }

        internal IntPtr mAddress = IntPtr.Zero;
    }

    [AttributeUsage(AttributeTargets.Class, AllowMultiple = false, Inherited = false)]
    public sealed class EventIDAttribute : Attribute
    {
        public EventIDAttribute(EventID id)
        {
            if (id == EventID.None)
            {
                throw new ArgumentException("Invalid event ID!");
            }

            ID = id;
        }

        public EventID ID { get; }
    }

    public sealed class EventDispatcher
    {
        public delegate bool DispatcherCallback<T>(T @event) where T : Event;

        public EventDispatcher(Event @event)
        {
            if (@event.ID != EventID.None)
            {
                mEvent = @event;
            }
        }

        public void Dispatch<T>(DispatcherCallback<T> callback) where T : Event
        {
            if (mEvent == null)
            {
                return;
            }

            Type requestedType = typeof(T);
            var attribute = requestedType.GetCustomAttribute<EventIDAttribute>();
            if (attribute == null)
            {
                return;
            }

            if (mEvent.ID == attribute.ID)
            {
                if (callback((T)mEvent))
                {
                    mEvent.Handled = true;
                }
            }
        }

        private readonly Event mEvent = null;
    }
}
