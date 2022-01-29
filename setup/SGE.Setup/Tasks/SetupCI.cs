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

namespace SGE.Setup.Tasks
{
    [Task("setup-ci")]
    [TaskDependency("install-vulkan", "--ci")]
    internal class SetupCI : Task
    {
        public override string Description => "Setup all required dependencies for a CI build";
        public override int Run(string[] args)
        {
            Console.WriteLine("Dependencies should be set up.");
            return 0;
        }
    }
}
