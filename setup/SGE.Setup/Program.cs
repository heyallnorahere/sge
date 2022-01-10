using System;
using System.Collections.Generic;
using System.CommandLine;
using System.CommandLine.Invocation;

namespace SGE.Setup
{
    public static class Program
    {
        private static string[]? TaskMenu(IReadOnlyDictionary<string, TaskInfo> tasks)
        {
            Console.WriteLine("Available tasks:");
            foreach (string name in tasks.Keys)
            {
                var task = tasks[name];
                string description = task.Instance.Description;
                Console.WriteLine($"\t{name}: {description}");
            }
            Console.Write("\nPlease enter a task name with arguments: ");

            string? argumentString = Console.ReadLine();
            if (argumentString == null || argumentString.Length == 0)
            {
                Console.WriteLine("No task was selected. Please try again.\n");
                return null;
            }

            var args = argumentString.Tokenize();
            if (args.Length == 0)
            {
                throw new ArgumentException("The given command was tokenized incorrectly!");
            }

            string taskName = args[0];
            if (tasks.ContainsKey(taskName))
            {
                return args;
            }

            Console.WriteLine($"Task \"{taskName}\" does not exist.\n");
            return null;
        }

        private static void TaskHandler(string task)
        {
            var tasks = Task.GetTasks();

            string taskName;
            string[] args;
            if (task.Length > 0)
            {
                if (!tasks.ContainsKey(task))
                {
                    throw new ArgumentException($"Task {task} does not exist.");
                }

                taskName = task;
                args = mUnmatchedTokens ?? Array.Empty<string>();
            }
            else
            {
                while (true)
                {
                    string[]? menuArgs = TaskMenu(tasks);
                    if (menuArgs != null)
                    {
                        taskName = menuArgs[0];
                        args = menuArgs[1..];
                        break;
                    }
                }
            }

            var context = new TaskContext(taskName, tasks);
            context.Execute(args);
        }

        public static int Main(string[] args)
        {
            var rootCommand = new RootCommand
            {
                new Option<string>(
                    new string[] { "-t", "--task" },
                    getDefaultValue: () => string.Empty,
                    description: "Execute a task and its dependencies."),
            };

            // really hacky but
            var result = rootCommand.Parse(args);
            var unmatchedTokens = result.UnmatchedTokens;
            if (unmatchedTokens.Count > 0)
            {
                mUnmatchedTokens = new string[unmatchedTokens.Count];
                for (int i = 0; i < unmatchedTokens.Count; i++)
                {
                    mUnmatchedTokens[i] = unmatchedTokens[i];
                }
            }

            rootCommand.TreatUnmatchedTokensAsErrors = false;
            rootCommand.Description = "Execute a setup task and its dependencies.";
            rootCommand.Handler = CommandHandler.Create(TaskHandler);

            return rootCommand.InvokeAsync(args).Result;
        }

        private static string[]? mUnmatchedTokens = null;
    }
}