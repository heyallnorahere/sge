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
using System.CommandLine;
using System.CommandLine.Invocation;
using System.Runtime.InteropServices;

namespace SGE.Setup.Tasks
{
    [Task("install-mono")]
    internal class InstallMono : Task
    {
        public override string Description => "Install Mono";
        public override int Run(string[] args)
        {
            var rootCommand = new RootCommand
            {
                new Option<bool>("--ci")
            };

            rootCommand.Description = Description;
            rootCommand.Handler = CommandHandler.Create(Install);

            return rootCommand.InvokeAsync(args).Result;
        }
        private const string MonoVersion = "6.12.0";
        private const string FullVersion = $"{MonoVersion}.107";
        private static bool InstallWindows()
        {
            string installerUrl = $"https://download.mono-project.com/archive/{MonoVersion}/windows-installer/mono-{FullVersion}-x64-0.msi";
            string? installerPath = Utilities.RetrieveFile(installerUrl, "mono.msi");
            if (installerPath == null)
            {
                return false;
            }

            var result = Utilities.ExecuteProcess("C:\\Windows\\System32\\msiexec.exe", $"/i {installerPath} /quiet", true);
            return result.ExitCode == 0;
        }
        private static bool InstallOSX()
        {
            string installerUrl = $"https://download.mono-project.com/archive/{MonoVersion}/macos-10-universal/MonoFramework-MDK-{FullVersion}.macos10.xamarin.universal.pkg";
            string? installerPath = Utilities.RetrieveFile(installerUrl, "mono.pkg");
            if (installerPath == null)
            {
                return false;
            }

            var result = Utilities.ExecuteProcess("sudo", $"installer -pkg {installerPath} -target /");
            return result.ExitCode == 0;
        }
        private static void Install(bool ci)
        {
            bool success = true;
            if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
            {
                Console.WriteLine("Installing Mono for Windows...");
                success = InstallWindows();
            }
            else if (RuntimeInformation.IsOSPlatform(OSPlatform.OSX))
            {
                Console.WriteLine("Installing Mono for MacOS X...");
                success = InstallOSX();
            }
            else
            {
                if (ci)
                {
                    Console.WriteLine("Installing Mono for Ubuntu 20.04...");
                    Utilities.RunCommand("sudo apt-get install -y gnupg ca-certificates");
                    Utilities.RunCommand("sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys 3FA7E0328081BFF6A14DA29AA6A19B38D3D831EF");
                    Utilities.RunCommand("bash -c \"echo \\\"deb https://download.mono-project.com/repo/ubuntu stable-focal main\\\" | sudo tee /etc/apt/sources.list.d/mono-official-stable.list\"");
                    Utilities.RunCommand("sudo apt-get update");
                    Utilities.RunCommand("sudo apt-get install -y mono-complete");
                }
                else
                {
                    Console.WriteLine("See https://www.mono-project.com/download/stable/#download-lin for installation instructions.");
                }
            }

            if (success)
            {
                Console.WriteLine("Successfully installed Mono!");
            }
            else
            {
                throw new Exception("Could not install Mono!");
            }
        }
    }
}
