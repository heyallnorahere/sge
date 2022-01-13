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
    [Task("install-vulkan")]
    internal class InstallVulkan : Task
    {
        public override string Description => "Install the Vulkan SDK";
        public override int Run(string[] args)
        {
            var rootCommand = new RootCommand
            {
                new Option<bool>("--ci")
            };

            rootCommand.Description = Description;
            rootCommand.Handler = CommandHandler.Create(InstallSDK);

            return rootCommand.InvokeAsync(args).Result;
        }
        private static void InstallSDK(bool ci)
        {
            bool success = true;
            if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
            {
                Console.WriteLine("Installing the Windows Vulkan SDK...");
                success = InstallWindows(ci);
            }
            else if (RuntimeInformation.IsOSPlatform(OSPlatform.OSX))
            {
                Console.WriteLine("Installing the MacOS X Vulkan SDK...");
                success = InstallOSX(ci);
            }
            else
            {
                if (ci)
                {
                    Console.WriteLine("Installing Vulkan for Debian-based systems...");
                    Utilities.RunCommand("sudo apt-get update");
                    Utilities.RunCommand("sudo apt-get install -y libvulkan-dev");
                }
                else
                {
                    Console.WriteLine("The Vulkan SDK can be installed via package manager on this platform - skipping");
                }
            }

            if (success)
            {
                Console.WriteLine("Successfully installed the Vulkan SDK!");
            }
            else
            {
                throw new Exception("Could not install the Vulkan SDK!");
            }
        }
        private const string SDKVersion = "1.2.182.0";
        private static bool InstallWindows(bool setActionsEnvVariable)
        {
            string installerUrl = $"https://sdk.lunarg.com/sdk/download/{SDKVersion}/windows/VulkanSDK-{SDKVersion}-Installer.exe";
            string? installerPath = Utilities.RetrieveFile(installerUrl, "vulkan-installer.exe");
            if (installerPath == null)
            {
                return false;
            }

            Utilities.RunCommand($"{installerPath} /S");

            if (setActionsEnvVariable)
            {
                Console.WriteLine($"::set-env name=VULKAN_SDK::C:\\VulkanSDK\\{SDKVersion}");
            }
            return true;
        }
        private static bool InstallOSX(bool setActionsEnvVariable)
        {
            string diskImageName = $"vulkansdk-macos-{SDKVersion}";
            string installerUrl = $"https://sdk.lunarg.com/sdk/download/{SDKVersion}/mac/{diskImageName}.dmg";
            string? diskImagePath = Utilities.RetrieveFile(installerUrl, $"{diskImageName}.dmg");
            if (diskImagePath == null)
            {
                return false;
            }

            Utilities.RunCommand($"sudo hdiutil attach {diskImagePath}");
            Utilities.RunCommand($"sudo /Volumes/{diskImageName}/InstallVulkan.app/Contents/MacOS/InstallVulkan in --al -c");

            if (setActionsEnvVariable)
            {
                string? homeDir = Environment.GetEnvironmentVariable("HOME");
                Console.WriteLine($"::set-env name=VULKAN_SDK::{homeDir}/VulkanSDK/{SDKVersion}/macOS");
            }
            return true;
        }
    }
}
