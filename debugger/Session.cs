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

    public abstract class EventHandler
    {
        public EventHandler()
        {
            mLock = new object();
        }

        protected abstract void HandleEventException(Exception exc);
        protected void HandleEvent(Action callback)
        {
            lock (mLock)
            {
                try
                {
                    callback();
                }
                catch (Exception exc)
                {
                    HandleEventException(exc);
                }
            }
        }


        protected T HandleEvent<T>(Func<T> callback)
        {
            lock (mLock)
            {
                try
                {
                    return callback();
                }
                catch (Exception exc)
                {
                    HandleEventException(exc);
                    return default;
                }
            }
        }

        internal void EnumerateEventMethods<T>(Action<MethodInfo, T> callback) where T : Attribute
        {
            const BindingFlags flags = BindingFlags.Instance |
                                       BindingFlags.NonPublic |
                                       BindingFlags.Public;

            var handlerType = GetType();
            var methods = handlerType.GetMethods(flags);

            foreach (var method in methods)
            {
                var attribute = method.GetCustomAttribute<T>();
                if (attribute == null)
                {
                    continue;
                }

                callback(method, attribute);
            }
        }

        private readonly object mLock;
    }

    public sealed class Session : IDisposable
    {
        private sealed class DebuggerEventHandler : EventHandler
        {
            public DebuggerEventHandler(Session session)
            {
                mSession = session;
            }

            protected override void HandleEventException(Exception exc)
            {
                var stackframe = new System.Diagnostics.StackFrame(1);
                var methodName = stackframe.GetMethod().Name;

                string excName = exc.GetType().FullName;
                Log.Error($"{excName} caught handling event {methodName}: {exc.Message}");
            }

            [DebuggerEvent]
            private void TargetStopped(object sender, TargetEventArgs args) => HandleEvent(() =>
            {
                Log.Info("Target stopped");
            });

            [DebuggerEvent]
            private void TargetHitBreakpoint(object sender, TargetEventArgs args) => HandleEvent(() =>
            {
                Log.Info("Target hit a breakpoint");
            });

            [DebuggerEvent]
            private void TargetExceptionThrown(object sender, TargetEventArgs args) => HandleEvent(() =>
            {
                Log.Info("An exception was thrown");
            });

            [DebuggerEvent]
            private void TargetUnhandledException(object sender, TargetEventArgs args) => HandleEvent(() =>
            {
                Log.Info("An unhandled exception was thrown");
            });

            [DebuggerEvent]
            private void TargetStarted(object sender, EventArgs args) => HandleEvent(() =>
            {
                Log.Info("Target started");
            });

            [DebuggerEvent]
            private void TargetReady(object sender, TargetEventArgs args) => HandleEvent(() =>
            {
                Log.Info("Target ready");
            });

            [DebuggerEvent]
            private void TargetExited(object sender, TargetEventArgs args) => HandleEvent(() =>
            {
                Log.Info("Target exited");
                mSession.mFrontend.Stop();
            });

            [DebuggerEvent]
            private void TargetInterrupted(object sender, TargetEventArgs args) => HandleEvent(() =>
            {
                Log.Info("Target interrupted");
            });

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
        }

        private sealed class ClientCommandHandler : EventHandler
        {
            public ClientCommandHandler(Session session)
            {
                mSession = session;
            }

            protected override void HandleEventException(Exception exc)
            {
                var stackframe = new System.Diagnostics.StackFrame(1);
                var method = stackframe.GetMethod();
                var attribute = method.GetCustomAttribute<CommandAttribute>();

                string excName = exc.GetType().FullName;
                Log.Error($"{excName} caught handling command {attribute.Name}: {exc.Message}");
            }

            [Command("testCommand")]
            private dynamic TestCommand(dynamic args) => HandleEvent(() => new
            {
                result = "this works!",
                passedArgs = args
            });

            private readonly Session mSession;
        }

        private sealed class CustomLogger : ICustomLogger
        {
            public void LogError(string message, Exception exc) => throw new NotImplementedException();
            public void LogAndShowException(string message, Exception exc) => throw new NotImplementedException();
            public void LogMessage(string format, params object[] args) => throw new NotImplementedException();
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

            using var session = new Session(ip, port);
            return session.Run();
        }

        private static void WriteOutput(string text, bool isStdErr, string source)
        {
            string message = $"{source}: {text}";
            var severity = isStdErr ? Log.Severity.Error : Log.Severity.Info;

            Log.Print(message, severity);
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

            mFrontend = new DebuggerFrontend(Port + 1); // lol
            mFrontend.SetHandler(new ClientCommandHandler(this));

            mDisposed = false;
            mSession = new SoftDebuggerSession
            {
                Breakpoints = new BreakpointStore(),
                ExceptionHandler = exc => true,
                LogWriter = (isStdErr, text) => WriteOutput(text, isStdErr, "Debugger"),
                OutputWriter = (isStdErr, text) => WriteOutput(text, isStdErr, "Debuggee")
            };

            var sessionType = mSession.GetType();
            var handler = new DebuggerEventHandler(this);
            handler.EnumerateEventMethods<DebuggerEventAttribute>((method, attribute) =>
            {
                var eventInfo = sessionType.GetEvent(method.Name);
                if (eventInfo == null)
                {
                    return;
                }

                var delegateType = eventInfo.EventHandlerType;
                var delegateObject = Delegate.CreateDelegate(delegateType, handler, method);

                eventInfo.AddEventHandler(mSession, delegateObject);
            });
        }

        public void Dispose()
        {
            if (mDisposed)
            {
                return;
            }

            mSession.Dispose();
            mDisposed = true;
        }

        private int Run()
        {
            DebuggerLoggingService.CustomLogger = new CustomLogger();
            if (!Connect())
            {
                return 1;
            }

            mFrontend.Run();
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

        private bool mDisposed;
        private readonly SoftDebuggerSession mSession;
        private readonly DebuggerFrontend mFrontend;
    }
}
