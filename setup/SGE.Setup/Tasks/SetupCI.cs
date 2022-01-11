using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SGE.Setup.Tasks
{
    [Task("setup-ci")]
    [TaskDependency("install-vulkan", "--ci")]
    [TaskDependency("install-mono", "--ci")]
    internal class SetupCI : Task
    {
        public override string Description => "Setup all required dependencies for a CI build";
        public override int Run(string[] args)
        {
            Console.WriteLine("Dependencies should be set up.");
            return 0;
        }
    }
}
