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

namespace SGE.Events
{
    [EventID(EventID.KeyPressed)]
    public sealed class KeyPressedEvent : Event
    {
        public KeyCode Key
        {
            get
            {
                KeyCode key;
                InternalCalls.GetPressedEventKey(mAddress, out key);
                return key;
            }
        }

        public int RepeatCount => InternalCalls.GetRepeatCount(mAddress);
    }

    [EventID(EventID.KeyReleased)]
    public sealed class KeyReleasedEvent : Event
    {
        public KeyCode Key
        {
            get
            {
                KeyCode key;
                InternalCalls.GetReleasedEventKey(mAddress, out key);
                return key;
            }
        }
    }

    [EventID(EventID.KeyTyped)]
    public sealed class KeyTypedEvent : Event
    {
        public KeyCode Key
        {
            get
            {
                KeyCode key;
                InternalCalls.GetTypedEventKey(mAddress, out key);
                return key;
            }
        }
    }

    [EventID(EventID.MouseMoved)]
    public sealed class MouseMovedEvent : Event
    {
        public Vector2 Position
        {
            get
            {
                Vector2 position;
                InternalCalls.GetEventMousePosition(mAddress, out position);
                return position;
            }
        }
    }

    [EventID(EventID.MouseScrolled)]
    public sealed class MouseScrolledEvent : Event
    {
        public Vector2 Offset
        {
            get
            {
                Vector2 offset;
                InternalCalls.GetScrollOffset(mAddress, out offset);
                return offset;
            }
        }
    }

    [EventID(EventID.MouseButton)]
    public sealed class MouseButtonEvent : Event
    {
        public MouseButton Button
        {
            get
            {
                MouseButton button;
                InternalCalls.GetEventMouseButton(mAddress, out button);
                return button;
            }
        }

        public bool Released => InternalCalls.GetMouseButtonReleased(mAddress);
    }
}
