using System;
using System.CommandLine;
using System.CommandLine.Invocation;
using System.Diagnostics;
using System.IO;
using System.Net.Http;
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
                Console.WriteLine("The Vulkan SDK can be installed via package manager on this platform - skipping");
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
            string? installerPath = RetrieveInstaller(installerUrl, "vulkan-installer.exe");
            if (installerPath == null)
            {
                return false;
            }

            var process = new Process()
            {
                StartInfo = new ProcessStartInfo
                {
                    FileName = installerPath,
                    Arguments = "/S",
                    UseShellExecute = true
                }
            };

            process.Start();
            process.WaitForExit();
            if (process.ExitCode != 0)
            {
                return false;
            }

            if (setActionsEnvVariable)
            {
                Console.WriteLine($"::set-env name=VULKAN_SDK::C:\\VulkanSDK\\{SDKVersion}");
            }
            return true;
        }
        private static bool InstallOSX(bool setActionsEnvVariable)
        {
            return true;
        }
        private static string? RetrieveInstaller(string installerUrl, string outputName)
        {
            try
            {
                var client = new HttpClient();
                var responseTask = client.GetAsync(installerUrl);
                responseTask.Wait();

                var response = responseTask.Result;
                response.EnsureSuccessStatusCode();

                var bodyTask = response.Content.ReadAsByteArrayAsync();
                bodyTask.Wait();
                var body = bodyTask.Result;

                string outputPath = Path.Join(Directory.GetCurrentDirectory(), outputName);
                var stream = new FileStream(outputPath, FileMode.OpenOrCreate, FileAccess.Write);
                stream.Write(body);
                stream.Close();

                return outputPath;
            }
            catch (Exception)
            {
                return null;
            }
        }
    }
}
