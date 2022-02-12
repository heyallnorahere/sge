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
    public sealed class PenguinScript : Script
    {
        public void OnEvent(Event @event)
        {
            var dispatcher = new EventDispatcher(@event);
            dispatcher.Dispatch<MouseButtonEvent>(OnMouseButton);
        }

        public bool OnMouseButton(MouseButtonEvent @event)
        {
            Vector2 mousePosition = Input.MousePosition;

            if (@event.Released) {
                Vector2 mouseDistance = mousePosition - mMouseStartPos;
                Vector2 force = mouseDistance * ForceModifier * (1f, -1f);

                if (HasComponent<RigidBodyComponent>())
                {
                    var rigidbody = GetComponent<RigidBodyComponent>();
                    rigidbody.AddForce(force);

                    Log.Info("Added force of ({0}, {1})", force.X, force.Y);
                }
                else
                {
                    Log.Warn("Could not add a force of ({0}, {1}) - no rigid body component attached", force.X, force.Y);
                }
            }
            else
            {
                mMouseStartPos = mousePosition;
            }

            return true;
        }

        public float ForceModifier { get; set; } = 10f;

        private Vector2 mMouseStartPos = (0f, 0f);
    }
}