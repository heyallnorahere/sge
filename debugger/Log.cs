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
using System.Diagnostics;
using System.IO;
using System.Runtime.CompilerServices;

namespace SGE.Debugger
{
    internal static class Log
    {
        public const string LogPath = "assets/logs/debugger.log";

        public enum Severity
        {
            Info,
            Warn,
            Error
        }

        private static readonly Stream sFile;
        private static readonly TextWriter sWriter;

        static Log()
        {
            string logDir = Path.GetDirectoryName(LogPath);
            if (logDir.Length > 0 && !Directory.Exists(logDir))
            {
                Directory.CreateDirectory(logDir);
            }

            sFile = new FileStream(LogPath, FileMode.Create);
            if (!sFile.CanWrite)
            {
                throw new IOException("Could not write to log file!");
            }

            sWriter = new StreamWriter(sFile);
        }

        private static void PrintText(string text, bool newline = false, bool stderr = false)
        {
            TextWriter consoleWriter = stderr ? Console.Error : Console.Out;
            if (newline)
            {
                sWriter.WriteLine(text);
                consoleWriter.WriteLine(text);
            }
            else
            {
                sWriter.Write(text);
                consoleWriter.Write(text);
            }
        }

        public static void Print(string message, Severity severity)
        {
            string date = DateTime.Now.ToString("MM-dd-yyyy HH:mm:ss");
            bool stderr = severity != Severity.Info;

            Console.ResetColor();
            PrintText($"[{date}] [SGE Debugger] [", stderr: stderr);

            Console.ForegroundColor = severity switch
            {
                Severity.Info => ConsoleColor.Green,
                Severity.Warn => ConsoleColor.Yellow,
                Severity.Error => ConsoleColor.Red,
                _ => throw new ArgumentException("Invalid severity!")
            };

            PrintText(severity.ToString().ToLower(), stderr: stderr);
            Console.ResetColor();

            PrintText($"] {message}", newline: true, stderr: stderr);
            sWriter.Flush();
        }

        private static void Print(string message)
        {
            var frame = new StackFrame(1);
            string name = frame.GetMethod().Name;

            var severity = (Severity)Enum.Parse(typeof(Severity), name);
            Print(message, severity);
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        public static void Info(string message) => Print(message);
        [MethodImpl(MethodImplOptions.NoInlining)]
        public static void Warn(string message) => Print(message);
        [MethodImpl(MethodImplOptions.NoInlining)]
        public static void Error(string message) => Print(message);
    }
}
