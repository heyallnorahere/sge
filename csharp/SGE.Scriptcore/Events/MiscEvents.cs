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

namespace SGE.Events
{
    public enum FileStatus : int
    {
        Created = 0,
        Deleted = 1,
        Modified = 2
    }

    [EventID(EventID.FileChanged)]
    public sealed class FileChangedEvent : Event
    {
        public string Path => CoreInternalCalls.GetChangedFilePath(mAddress);
        public string WatchedDirectory => CoreInternalCalls.GetWatchedDirectory(mAddress);

        public FileStatus Status
        {
            get
            {
                CoreInternalCalls.GetFileStatus(mAddress, out FileStatus status);
                return status;
            }
        }
    }
}
