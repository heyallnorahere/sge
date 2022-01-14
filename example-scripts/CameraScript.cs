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
using System;

namespace ExampleScripts
{
    public sealed class CameraScript : Script
    {
        public void OnStart()
        {
            var tagComponent = GetComponent<TagComponent>();
            Console.WriteLine($"{tagComponent.Tag}: started scene");
        }
        public void OnUpdate(Timestep ts)
        {
            if (Target != null)
            {
                var targetTransform = Target.GetComponent<TransformComponent>();
                var transform = GetComponent<TransformComponent>();
                transform.Translation = targetTransform.Translation;
            }
        }
        public Entity Target { get; set; } = null;
    }
}