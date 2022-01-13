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
using System.Diagnostics;
using System.IO;
using System.Net.Http;
using System.Reflection;

namespace SGE.Setup
{
    struct ProcessResult
    {
        public int ExitCode;
        public string StandardOutput;
        public string StandardError;
    };

    internal static class Utilities
    {
        public static string? RetrieveFile(string url, string outputName)
        {
            try
            {
                var client = new HttpClient();
                var responseTask = client.GetAsync(url);
                responseTask.Wait();

                var response = responseTask.Result;
                response.EnsureSuccessStatusCode();

                var bodyTask = response.Content.ReadAsByteArrayAsync();
                bodyTask.Wait();
                var body = bodyTask.Result;

                var assembly = Assembly.GetExecutingAssembly();
                var assemblyPath = assembly.Location;
                var assemblyDirectory = Path.GetDirectoryName(assemblyPath);

                string outputPath = Path.Join(assemblyDirectory, outputName);
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
        public static ProcessResult ExecuteProcess(string fileName, string arguments, bool shell = false)
        {
            var process = new Process()
            {
                StartInfo = new ProcessStartInfo
                {
                    FileName = fileName,
                    Arguments = arguments,
                    UseShellExecute = shell,
                    RedirectStandardOutput = !shell,
                    RedirectStandardError = !shell
                }
            };

            process.Start();
            process.WaitForExit();

            string standardOutput = string.Empty;
            string standardError = string.Empty;
            if (!shell)
            {
                standardOutput = process.StandardOutput.ReadToEnd();
                standardError = process.StandardError.ReadToEnd();
            }

            return new ProcessResult
            {
                ExitCode = process.ExitCode,
                StandardOutput = standardOutput,
                StandardError = standardError
            };
        }
        public static void RunCommand(string command)
        {
            string filename = command;
            string arguments = string.Empty;

            int firstSpace = command.IndexOf(' ');
            if (firstSpace >= 0)
            {
                filename = command.Substring(0, firstSpace);
                arguments = command.Substring(firstSpace + 1);
            }

            int exitCode = ExecuteProcess(filename, arguments, true).ExitCode;
            if (exitCode != 0)
            {
                throw new Exception($"Command \"{command}\" exited with code {exitCode}.");
            }
        }
        public static bool Extends(this Type derived, Type baseType)
        {
            Type? currentBaseType = derived.BaseType;
            if (currentBaseType != null)
            {
                if (currentBaseType == baseType)
                {
                    return true;
                }
                else
                {
                    return currentBaseType.Extends(baseType);
                }
            }

            return false;
        }

        public static string[] Tokenize(this string command)
        {
            string[] sections = command.Split(' ', StringSplitOptions.RemoveEmptyEntries);

            string? currentToken = null;
            var completedTokens = new List<string>();

            foreach (string section in sections)
            {
                bool containsQuote = false;
                bool escapeNextCharacter = false;

                string token = string.Empty;
                for (int i = 0; i < section.Length; i++)
                {
                    char character = section[i];

                    switch (character)
                    {
                        case '"':
                            if (escapeNextCharacter)
                            {
                                token += character;
                                escapeNextCharacter = false;
                            }
                            else
                            {
                                containsQuote = !containsQuote;
                            }
                            break;
                        case '\\':
                            if (i == section.Length - 1)
                            {
                                containsQuote = true;
                            }
                            else if (!escapeNextCharacter)
                            {
                                escapeNextCharacter = true;
                            }
                            else
                            {
                                escapeNextCharacter = false;
                                token += character;
                            }
                            break;
                        default:
                            if (escapeNextCharacter)
                            {
                                throw new ArgumentException($"cannot escape character: {character}");
                            }
                            else
                            {
                                token += character;
                            }
                            break;
                    }
                }

                bool beganToken = false;
                if (currentToken == null && containsQuote)
                {
                    currentToken = string.Empty;
                    beganToken = true;
                }

                if (currentToken != null)
                {
                    currentToken += " " + token;
                }
                else
                {
                    completedTokens.Add(token);
                }

                if (containsQuote && !beganToken && currentToken != null)
                {
                    completedTokens.Add(currentToken);
                    currentToken = null;
                }
            }

            if (currentToken != null)
            {
                completedTokens.Add(currentToken);
            }

            return completedTokens.ToArray();
        }
    }
}
