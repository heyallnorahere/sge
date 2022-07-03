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

using Mono.Debugging.Client;
using Mono.Debugging.Soft;
using System;
using System.Collections.Generic;
using System.Net;
using System.Reflection;

namespace SGE.Debugger
{
    [AttributeUsage(AttributeTargets.Method, AllowMultiple = false, Inherited = false)]
    internal sealed class DebuggerEventAttribute : Attribute
    {
        // nothing here - just a flag
    }

    public sealed class Session
    {
        private sealed class DebuggerEventHandler
        {
            public DebuggerEventHandler(Session session)
            {
                mSession = session;
                mLock = new object();
            }

            private void HandleEvent(Action callback)
            {
                lock (mLock)
                {
                    mSession.mEventHandling = true;

                    try
                    {
                        callback();
                    }
                    catch (Exception exc)
                    {
                        var stackframe = new System.Diagnostics.StackFrame(1);
                        var methodName = stackframe.GetMethod().Name;

                        string excName = exc.GetType().FullName;
                        Log.Error($"{excName} caught handling event {methodName}: {exc.Message}");
                    }

                    mSession.mEventHandling = false;
                }
            }

            [DebuggerEvent]
            private void TargetStopped(object sender, TargetEventArgs args)
            {
                HandleEvent(() =>
                {
                    Log.Info("Target stopped");
                });
            }

            [DebuggerEvent]
            private void TargetHitBreakpoint(object sender, TargetEventArgs args)
            {
                HandleEvent(() =>
                {
                    Log.Info("Target hit a breakpoint");
                });
            }

            [DebuggerEvent]
            private void TargetExceptionThrown(object sender, TargetEventArgs args)
            {
                HandleEvent(() =>
                {
                    Log.Info("An exception was thrown");
                });
            }

            [DebuggerEvent]
            private void TargetUnhandledException(object sender, TargetEventArgs args)
            {
                HandleEvent(() =>
                {
                    Log.Info("An unhandled exception was thrown");
                });
            }

            [DebuggerEvent]
            private void TargetStarted(object sender, EventArgs args)
            {
                HandleEvent(() =>
                {
                    Log.Info("Target started");
                });
            }

            [DebuggerEvent]
            private void TargetReady(object sender, TargetEventArgs args)
            {
                HandleEvent(() =>
                {
                    Log.Info("Target ready");
                });
            }

            [DebuggerEvent]
            private void TargetExited(object sender, TargetEventArgs args)
            {
                HandleEvent(() =>
                {
                    Log.Info("Target exited");
                });
            }

            [DebuggerEvent]
            private void TargetInterrupted(object sender, TargetEventArgs args)
            {
                HandleEvent(() =>
                {
                    Log.Info("Target interrupted");
                });
            }

            [DebuggerEvent]
            private void TargetThreadStarted(object sender, TargetEventArgs args)
            {
                HandleEvent(() =>
                {
                    Log.Info("Target started a thread");
                });
            }

            [DebuggerEvent]
            private void TargetThreadStopped(object sender, TargetEventArgs args)
            {
                HandleEvent(() =>
                {
                    Log.Info("Target stopped a thread");
                });
            }

            private readonly Session mSession;
            private readonly object mLock;
        }

        private sealed class CustomLogger : ICustomLogger
        {
            public void LogError(string message, Exception exc)
            {
                Log.Error(message);
                if (exc != null)
                {
                    Log.Error(exc.ToString());
                }
            }

            public void LogAndShowException(string message, Exception exc) => LogError(message, exc);
            public void LogMessage(string format, params object[] args) => Log.Info(format, args);

            public string GetNewDebuggerLogFilename() => null;
        }

        private const int MaxConnectionAttempts = 20;
        private const int ConnectionInterval = 5000;

        public static int Start(ArgumentParser.Result args)
        {
            var supportedPlatforms = new HashSet<PlatformID>
            {
                PlatformID.MacOSX,
                PlatformID.Unix,
                PlatformID.Win32NT
            };

            PlatformID platform = Environment.OSVersion.Platform;
            if (!supportedPlatforms.Contains(platform))
            {
                Log.Error($"Platform not supported: {platform}");
                return 1;
            }

            string ip = args.Get("address");
            int port = int.Parse(args.Get("port"));

            var session = new Session(ip, port);
            return session.Run();
        }

        private Session(string ip, int port)
        {
            var address = Utilities.ResolveIP(ip);
            if (address == null)
            {
                throw new ArgumentException("Invalid IP address!");
            }

            Address = address;
            Port = port;

            mEventHandling = false;
            mSession = new SoftDebuggerSession
            {
                Breakpoints = new BreakpointStore(),
                ExceptionHandler = exc => true,
                LogWriter = (isStdErr, text) => { },
                OutputWriter = (isStdErr, text) => Log.Print(text, isStdErr ? Log.Severity.Error : Log.Severity.Info)
            };

            var handler = new DebuggerEventHandler(this);
            var handlerType = handler.GetType();
            var methods = handlerType.GetMethods(BindingFlags.Instance | BindingFlags.NonPublic);

            var sessionType = mSession.GetType();
            foreach (var method in methods)
            {
                if (method.GetCustomAttribute<DebuggerEventAttribute>() == null)
                {
                    continue;
                }

                var eventInfo = sessionType.GetEvent(method.Name);
                if (eventInfo == null)
                {
                    continue;
                }

                var delegateType = eventInfo.EventHandlerType;
                var delegateObject = Delegate.CreateDelegate(delegateType, handler, method);

                eventInfo.AddEventHandler(mSession, delegateObject);
            }
        }

        private int Run()
        {
            DebuggerLoggingService.CustomLogger = new CustomLogger();
            if (!Connect())
            {
                return 1;
            }

            var frontend = new DebuggerFrontend(Port + 1); // lol
            frontend.Run();

            while (mSession.IsRunning)
            {
                // todo: something
            }

            frontend.Stop();
            return 0;
        }

        private bool Connect()
        {
            Log.Info($"Attempting to connect to {Address}:{Port}...");

            var args = new SoftDebuggerConnectArgs(string.Empty, Address, Port)
            {
                MaxConnectionAttempts = MaxConnectionAttempts,
                TimeBetweenConnectionAttempts = ConnectionInterval
            };

            var sessionOptions = new DebuggerSessionOptions
            {
                EvaluationOptions = EvaluationOptions.DefaultOptions
            };

            var startInfo = new SoftDebuggerStartInfo(args);
            mSession.Run(startInfo, sessionOptions);

            if (mSession.IsRunning)
            {
                Log.Info($"Successfully connected to {Address}:{Port}!");
                return true;
            }
            else
            {
                Log.Error($"Failed to connect to {Address}:{Port}!");
                return false;
            }
        }

        public IPAddress Address { get; }
        public int Port { get; }

        private readonly SoftDebuggerSession mSession;
        private readonly DebuggerFrontend mFrontend;
        private bool mEventHandling;
    }
}
