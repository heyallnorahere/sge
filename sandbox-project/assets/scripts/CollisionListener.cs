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

using SGE;
using SGE.Components;
using SGE.Events;

namespace Sandbox
{
    public sealed class CollisionListener : Script
    {
        public void OnCollision(Entity other)
        {
            var tag = GetComponent<TagComponent>().Tag;
            var otherTag = other.GetComponent<TagComponent>().Tag;

            Log.Info("{0}: collided with {1}", tag, otherTag);
            TotalCollisions++;
        }

        public void OnEvent(Event @event)
        {
            var dispatcher = new EventDispatcher(@event);
            dispatcher.Dispatch<KeyPressedEvent>(OnKey);
        }

        private bool OnKey(KeyPressedEvent @event)
        {
            if (@event.RepeatCount > 0 || @event.Key != KeyCode.C)
            {
                return false;
            }

            string tag = GetComponent<TagComponent>().Tag;
            Log.Info("{0}: collided with an object {1} total times.", tag, TotalCollisions);

            return true;
        }

        public int TotalCollisions { get; set; } = 0;
    }
}