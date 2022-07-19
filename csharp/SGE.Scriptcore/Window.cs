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

namespace SGE
{
    public enum DialogMode
    {
        Open = 0,
        Save
    }

    public sealed class DialogFilter
    {
        public DialogFilter(string filter)
        {
            Name = Filter = filter;
        }

        public DialogFilter(string name, string filter)
        {
            Name = name;
            Filter = filter;
        }

        public string Name { get; }
        public string Filter { get; }
    }

    public sealed class Window
    {
        public Window(string title, uint width, uint height)
        {
            CoreInternalCalls.CreateWindow(title, width, height, out mAddress);
            if (mAddress == IntPtr.Zero)
            {
                throw new ArgumentException("Could not create window!");
            }
        }

        internal Window(IntPtr address)
        {
            mAddress = address;
            CoreInternalCalls.AddRef_window(mAddress);
        }

        ~Window() => CoreInternalCalls.RemoveRef_window(mAddress);

        public uint Width => CoreInternalCalls.GetWindowWidth(mAddress);
        public uint Height => CoreInternalCalls.GetWindowHeight(mAddress);

        public string FileDialog(DialogMode mode, IEnumerable<DialogFilter> filters)
        {
            var filterList = new List<DialogFilter>(filters);
            return CoreInternalCalls.WindowFileDialog(mAddress, mode, filterList);
        }

        private readonly IntPtr mAddress;
    }
}
