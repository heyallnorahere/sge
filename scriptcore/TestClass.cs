using System;

namespace SGE
{
    public class TestClass
    {
        public TestClass(int x)
        {
            mX = x;
        }

        public void TestMethod(int y)
        {
            Console.WriteLine("If this message printed, we're good!");
            Console.WriteLine($"{mX} * {y} = {mX * y}");
        }

        private int mX;
    }
}