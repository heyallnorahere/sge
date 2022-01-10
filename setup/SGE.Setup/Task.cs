using System;
using System.Collections.Generic;
using System.Reflection;

namespace SGE.Setup
{
    [AttributeUsage(AttributeTargets.Class, AllowMultiple = false, Inherited = false)]
    internal sealed class TaskAttribute : Attribute
    {
        public TaskAttribute(string name, params object?[] constructorParams)
        {
            Name = name;
            ConstructorParams = constructorParams;
        }
        public string Name { get; }
        public object?[] ConstructorParams { get; }
    }

    [AttributeUsage(AttributeTargets.Class, AllowMultiple = true, Inherited = false)]
    internal sealed class TaskDependencyAttribute : Attribute
    {
        public TaskDependencyAttribute(string taskName, params string[] taskArguments)
        {
            TaskName = taskName;
            TaskArguments = taskArguments;
        }
        public string TaskName { get; }
        public string[] TaskArguments { get; }
    }

    internal struct TaskDependencyInfo
    {
        public string TaskName;
        public string[] TaskArguments;
    }

    internal struct TaskInfo
    {
        public Task Instance;
        public TaskDependencyInfo[] Dependencies;
    }

    internal abstract class Task
    {
        public virtual string Description => "A task that does amazing things.";
        public abstract int Run(string[] args);

        public static IReadOnlyDictionary<string, TaskInfo> GetTasks()
        {
            var tasks = new Dictionary<string, TaskInfo>();

            var assembly = typeof(Task).Assembly;
            foreach (var type in assembly.GetTypes())
            {
                var attribute = type.GetCustomAttribute<TaskAttribute>();
                if (attribute != null)
                {
                    if (!type.Extends(typeof(Task)))
                    {
                        throw new ArgumentException($"Type {type.FullName} has an instance of TaskAttribute, but does not extend Task!");
                    }

                    if (tasks.ContainsKey(attribute.Name))
                    {
                        throw new ArgumentException($"A task named \"{attribute.Name}\" already exists!");
                    }

                    var argumentTypes = new Type[attribute.ConstructorParams.Length];
                    for (int i = 0; i < argumentTypes.Length; i++)
                    {
                        object? argumentValue = attribute.ConstructorParams[i];

                        Type argumentType = typeof(object);
                        if (argumentValue != null)
                        {
                            argumentType = argumentValue.GetType();
                        }

                        argumentTypes[i] = argumentType;
                    }

                    var constructor = type.GetConstructor(argumentTypes);
                    if (constructor == null)
                    {
                        throw new ArgumentException("Could not find a suitable constructor!");
                    }

                    var dependencyAttributes = type.GetCustomAttributes<TaskDependencyAttribute>();
                    var dependencies = new List<TaskDependencyInfo>();
                    foreach (var dependencyAttribute in dependencyAttributes)
                    {
                        dependencies.Add(new TaskDependencyInfo
                        {
                            TaskName = dependencyAttribute.TaskName,
                            TaskArguments = dependencyAttribute.TaskArguments
                        });
                    }

                    tasks.Add(attribute.Name, new TaskInfo
                    {
                        Instance = (Task)constructor.Invoke(attribute.ConstructorParams),
                        Dependencies = dependencies.ToArray()
                    });
                }
            }

            return tasks;
        }
    }
}
