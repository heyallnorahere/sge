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
using System.Reflection;

namespace SGE.Debugger
{
    public class ArgumentParser
    {
        public sealed class Result
        {
            public Result(IReadOnlyDictionary<string, string> options, IReadOnlyList<string> unparsed)
            {
                OptionValues = options;
                UnparsedValues = unparsed;
            }

            public string Get(string optionName)
            {
                if (!OptionValues.ContainsKey(optionName))
                {
                    throw new ArgumentException($"Key does not exist: {optionName}");
                }

                return OptionValues[optionName];
            }

            public bool TryGet(string optionName, out string optionValue)
            {
                if (OptionValues.ContainsKey(optionName))
                {
                    optionValue = OptionValues[optionName];
                    return true;
                }
                else
                {
                    optionValue = string.Empty;
                    return false;
                }
            }

            public IReadOnlyDictionary<string, string> OptionValues { get; }
            public IReadOnlyList<string> UnparsedValues { get; }
        }

        public ArgumentParser()
        {
            var assembly = Assembly.GetExecutingAssembly();

            Name = assembly.FullName;
            Description = string.Empty;

            mOptions = new Dictionary<string, string>();
        }

        public string Name { get; set; }
        public string Description { get; set; }

        public IReadOnlyDictionary<string, string> OptionDescriptions => mOptions;

        public bool AddOption(string name) => AddOption(name, string.Empty);
        public bool RemoveOption(string name) => mOptions.Remove(name);

        public bool AddOption(string name, string description)
        {
            if (mOptions.ContainsKey(name))
            {
                return false;
            }

            mOptions.Add(name, description);
            return true;
        }

        private void PrintHelp()
        {
            string descString = Name;
            if (Description.Length > 0)
            {
                descString += " - " + Description;
            }

            var lines = new List<string>
            {
                descString,
                string.Empty,
                "Options:"
            };

            var optionDisplays = new List<Tuple<string, string>>();
            foreach (string optionName in mOptions.Keys)
            {
                string usage = optionName + " <value>";
                optionDisplays.Add(new Tuple<string, string>(usage, mOptions[optionName]));
            }

            int longestUsage = 0;
            optionDisplays.ForEach(display =>
            {
                string usage = display.Item1;
                if (usage.Length > longestUsage)
                {
                    longestUsage = usage.Length;
                }
            });

            // ...again
            foreach (var display in optionDisplays)
            {
                string line = display.Item1;

                string description = display.Item2;
                if (description.Length > 0)
                {
                    int spaceCount = longestUsage - line.Length + 4;
                    for (int i = 0; i < spaceCount; i++)
                    {
                        line += ' ';
                    }

                    line += description;
                }

                lines.Add(line);
            }

            lines.ForEach(Console.WriteLine);
        }

        public int Parse(string[] args, Func<Result, int> callback)
        {
            var optionValues = new Dictionary<string, string>();
            var unparsedValues = new List<string>();

            foreach (string argument in args)
            {
                if (argument.StartsWith("--"))
                {
                    var argumentData = argument.Substring(2);

                    int delimiterPos = -1;
                    for (int i = 0; i < argumentData.Length; i++)
                    {
                        if (argumentData[i] == '=')
                        {
                            delimiterPos = i;
                            break;
                        }
                    }

                    if (delimiterPos > 0)
                    {
                        string optionName = argumentData.Substring(0, delimiterPos);
                        if (mOptions.ContainsKey(optionName) && !optionValues.ContainsKey(optionName))
                        {
                            string optionValue = argumentData.Substring(delimiterPos + 1);
                            optionValues.Add(optionName, optionValue);

                            continue;
                        }
                    }
                    else if (argumentData == "help")
                    {
                        PrintHelp();
                        return 0;
                    }
                }

                unparsedValues.Add(argument);
            }

            var result = new Result(optionValues, unparsedValues);
            return callback(result);
        }

        public void Parse(string[] args, Action<Result> callback)
        {
            Parse(args, result =>
            {
                callback(result);
                return 0;
            });
        }

        private readonly Dictionary<string, string> mOptions;
    }

    public static class Program
    {
        public static int Main(string[] args)
        {
            try
            {
                var parser = new ArgumentParser
                {
                    Name = "SGE Debugger",
                    Description = "Debug the embedded mono host used in SGE"
                };

                parser.AddOption("address", "The address to connect to.");
                parser.AddOption("port", "The port to connect to.");
                parser.AddOption("connect", "Whether or not to connect or host");

                return parser.Parse(args, Session.Start);
            }
            catch (Exception exc)
            {
                Log.Error($"{exc.GetType().FullName} thrown: {exc.Message}");
                Log.Error(exc.StackTrace);
                return 1;
            }
        }
    }
}