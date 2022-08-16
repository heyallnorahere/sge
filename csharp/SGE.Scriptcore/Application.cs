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

namespace SGE
{
    public enum Subsystem : uint
    {
        Input = 0x01,
        Asset = 0x02,
        Project = 0x04,
        ScriptEngine = 0x08,
        Sound = 0x10
    }

    public static class Application
    {
        public static string EngineVersion => CoreInternalCalls.GetEngineVersion();
        public static void Quit() => CoreInternalCalls.QuitApplication();
        public static string Title => CoreInternalCalls.GetApplicationTitle();

        public static Window MainWindow
        {
            get
            {
                CoreInternalCalls.GetMainWindow(out IntPtr pointer);
                return new Window(pointer);
            }
        }

        public static bool IsEditor => CoreInternalCalls.IsApplicationEditor();
        public static bool IsSubsystemInitialized(Subsystem subsystem) => CoreInternalCalls.IsSubsystemInitialized(subsystem);
    }
}
