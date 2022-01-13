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
using System.Linq;

namespace SGE.Setup
{
    internal class TaskContext
    {
        public TaskContext(string startTask, IReadOnlyDictionary<string, TaskInfo> tasks)
        {
            mStartTask = startTask;
            mTasks = tasks;
            mDependencyTree = new Dictionary<string, DependencyTreeNode>();

            ProcessTaskNode(mStartTask);
        }

        public void Execute(IEnumerable<string> args)
        {
            var node = mDependencyTree[mStartTask];
            node.Arguments.Clear();
            foreach (string arg in args)
            {
                node.Arguments.Add(arg);
            }

            var executedTasks = new HashSet<string>();
            Execute(mStartTask, executedTasks);
        }

        private void ProcessTaskNode(string taskName)
        {
            if (!mTasks.ContainsKey(taskName))
            {
                throw new ArgumentException($"Task {taskName} does not exist.");
            }

            if (mDependencyTree.ContainsKey(taskName) &&
                mDependencyTree[taskName].Processed)
            {
                return;
            }

            if (!mDependencyTree.ContainsKey(taskName))
            {
                mDependencyTree.Add(taskName, new DependencyTreeNode());
            }

            var node = mDependencyTree[taskName];
            node.Processed = true;

            var nodeInfo = mTasks[taskName];
            foreach (var dependency in nodeInfo.Dependencies)
            {
                node.Dependencies.Add(dependency.TaskName);

                if (!mDependencyTree.ContainsKey(dependency.TaskName))
                {
                    mDependencyTree.Add(dependency.TaskName, new DependencyTreeNode());
                }

                var dependencyNode = mDependencyTree[dependency.TaskName];
                dependencyNode.Dependents.Add(taskName);
                
                foreach (string argument in dependency.TaskArguments)
                {
                    dependencyNode.Arguments.Add(argument);
                }

                ProcessTaskNode(dependency.TaskName);
            }
        }

        private void Execute(string taskName, HashSet<string> executedTasks)
        {
            var node = mDependencyTree[taskName];
            foreach (var dependency in node.Dependencies)
            {
                if (executedTasks.Contains(dependency))
                {
                    continue;
                }

                Execute(dependency, executedTasks);
            }

            var argumentArray = node.Arguments.ToArray();
            Task instance = mTasks[taskName].Instance;

            Console.WriteLine($"Running task: {taskName}...");
            int result = instance.Run(argumentArray);
            if (result != 0)
            {
                throw new ArgumentException($"Task {taskName} returned code {result}.");
            }

            if (taskName != mStartTask)
            {
                // if this task ran as a dependency, add a newline to make it look cleaner.
                Console.Write('\n');
            }
            executedTasks.Add(taskName);
        }

        private class DependencyTreeNode
        {
            public DependencyTreeNode()
            {
                Dependencies = new HashSet<string>();
                Dependents = new HashSet<string>();
                Arguments = new HashSet<string>();
                Processed = false;
            }

            public HashSet<string> Dependencies { get; }
            public HashSet<string> Dependents { get; }
            public HashSet<string> Arguments { get; }
            public bool Processed { get; set; }
        }

        private readonly string mStartTask;
        private readonly IReadOnlyDictionary<string, TaskInfo> mTasks;
        private readonly Dictionary<string, DependencyTreeNode> mDependencyTree;
    }
}
