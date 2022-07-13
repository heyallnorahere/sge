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

using SGE.Components;
using System.Collections.Generic;

namespace SGE
{
    /// <summary>
    /// A script is an object that can be applied to an entity to give custom behavior.
    /// </summary>
    public abstract class Script
    {
        protected bool HasComponent<T>() => __internal_mEntity.HasComponent<T>();
        protected T GetComponent<T>() where T : Component<T>, new() => __internal_mEntity.GetComponent<T>();

        public Entity Entity => __internal_mEntity;
        private Entity __internal_mEntity = null;
    }
}