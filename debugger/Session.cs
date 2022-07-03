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
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Reflection;
using System.Threading;

namespace SGE.Debugger
{
    [AttributeUsage(AttributeTargets.Method, AllowMultiple = false, Inherited = false)]
    internal sealed class DebuggerEventAttribute : Attribute
    {
        // nothing here - just a flag
    }

    public abstract class EventHandler
    {
        public const dynamic Null = null;

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

    internal sealed class DebuggerThread
    {
        public DebuggerThread(long id, string name)
        {
            ID = id;
            Name = name;
        }

        [JsonProperty(PropertyName = "id")]
        public long ID { get; }

        [JsonProperty(PropertyName = "name")]
        public string Name { get; }
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
                mSession.OnStopped();
                mSession.mFrontend.SendEvent(SocketEventType.DebuggerStep, new
                {
                    thread = args.Thread.Id,
                    message = Null
                });

                mSession.mResumeEvent.Set();
                Log.Info("Target stopped");
            });

            [DebuggerEvent]
            private void TargetHitBreakpoint(object sender, TargetEventArgs args) => HandleEvent(() =>
            {
                mSession.OnStopped();
                mSession.mFrontend.SendEvent(SocketEventType.BreakpointHit, new
                {
                    thread = args.Thread.Id,
                    message = Null
                });

                mSession.mResumeEvent.Set();
                Log.Info("Target hit a breakpoint");
            });

            private void OnExceptionThrown(TargetEventArgs args, SocketEventType eventType)
            {
                mSession.OnStopped();

                var exception = mSession.ActiveException;
                if (exception != null)
                {
                    mSession.mCurrentException = exception.Instance;
                    mSession.mFrontend.SendEvent(eventType, new
                    {
                        thread = args.Thread.Id,
                        message = exception.Message
                    });
                }

                mSession.mResumeEvent.Set();
            }

            [DebuggerEvent]
            private void TargetExceptionThrown(object sender, TargetEventArgs args) => HandleEvent(() =>
            {
                OnExceptionThrown(args, SocketEventType.HandledExceptionThrown);
                Log.Info("An handled exception was thrown");
            });

            [DebuggerEvent]
            private void TargetUnhandledException(object sender, TargetEventArgs args) => HandleEvent(() =>
            {
                OnExceptionThrown(args, SocketEventType.UnhandledExceptionThrown);
                Log.Info("An unhandled exception was thrown");
            });

            [DebuggerEvent]
            private void TargetStarted(object sender, EventArgs args) => HandleEvent(() =>
            {
                mSession.mActiveFrame = null;
                Log.Info("Target started");
            });

            [DebuggerEvent]
            private void TargetReady(object sender, TargetEventArgs args) => HandleEvent(() =>
            {
                mSession.mActiveProcess = mSession.mSession.GetProcesses().SingleOrDefault();
                Log.Info("Target ready");
            });

            [DebuggerEvent]
            private void TargetExited(object sender, TargetEventArgs args) => HandleEvent(() =>
            {
                Log.Info("Target exited");
                mSession.mResumeEvent.Set();
                mSession.mFrontend.Stop();
            });

            [DebuggerEvent]
            private void TargetInterrupted(object sender, TargetEventArgs args) => HandleEvent(() =>
            {
                mSession.mResumeEvent.Set();
                Log.Info("Target interrupted");
            });

            [DebuggerEvent]
            private void TargetThreadStarted(object sender, TargetEventArgs args) => HandleEvent(() =>
            {
                long threadId = args.Thread.Id;
                string threadName = args.Thread.Name;

                lock (mSession.mSeenThreads)
                {
                    var thread = new DebuggerThread(threadId, threadName);
                    mSession.mSeenThreads.Add(threadId, thread);
                }

                mSession.mFrontend.SendEvent(SocketEventType.ThreadStarted, threadId);
                Log.Info($"Target started a thread: {threadName}");
            });

            [DebuggerEvent]
            private void TargetThreadStopped(object sender, TargetEventArgs args) => HandleEvent(() =>
            {
                long threadId = args.Thread.Id;
                string threadName = mSession.mSeenThreads[threadId].Name;

                lock (mSession.mSeenThreads)
                {
                    mSession.mSeenThreads.Remove(threadId);
                }

                mSession.mFrontend.SendEvent(SocketEventType.ThreadExited, threadId);
                Log.Info($"Target stopped a thread: {threadName}");
            });

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

            [Command("next")]
            private dynamic Next(dynamic args) => HandleEvent(() =>
            {
                return Null;
            });

            [Command("continue")]
            private dynamic Continue(dynamic args) => HandleEvent(() =>
            {
                return Null;
            });

            [Command("stepIn")]
            private dynamic StepIn(dynamic args) => HandleEvent(() =>
            {
                return Null;
            });

            [Command("stepOut")]
            private dynamic StepOut(dynamic args) => HandleEvent(() =>
            {
                return Null;
            });

            [Command("pause")]
            private dynamic Pause(dynamic args) => HandleEvent(() =>
            {
                return Null;
            });

            [Command("stackTrace")]
            private dynamic StackTrace(dynamic args) => HandleEvent(() =>
            {
                return Null;
            });

            [Command("scopes")]
            private dynamic Scopes(dynamic args) => HandleEvent(() =>
            {
                return Null;
            });

            [Command("threads")]
            private dynamic Threads(dynamic args) => HandleEvent(() =>
            {
                return Null;
            });

            [Command("setBreakpoints")]
            private dynamic SetBreakpoints(dynamic args) => HandleEvent(() =>
            {
                return Null;
            });

            [Command("setFunctionBreakpoints")]
            private dynamic SetFunctionBreakpoints(dynamic args) => HandleEvent(() =>
            {
                return Null;
            });

            [Command("setExceptionBreakpoints")]
            private dynamic SetExceptionBreakpoints(dynamic args) => HandleEvent(() =>
            {
                return Null;
            });

            [Command("evaluate")]
            private dynamic Evaluate(dynamic args) => HandleEvent(() =>
            {
                return Null;
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

        private const int MaxConnectionAttempts = 50;
        private const int ConnectionInterval = 500;

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
            for (int i = message.Length - 1; i >= 0; i--)
            {
                char character = message[i];
                if (character != '\r' && character != '\n')
                {
                    message = message.Substring(0, i + 1);
                    break;
                }
            }

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

            mVariableHandles = new ObjectRegistry<ObjectValue[]>();
            mFrameHandles = new ObjectRegistry<StackFrame>();
            mCurrentException = null;
            mSeenThreads = new Dictionary<long, DebuggerThread>();

            mDisposed = false;
            mLock = new object();

            mResumeEvent = new AutoResetEvent(false);
            mDebuggeeRunning = false;

            mFrontend = new DebuggerFrontend(Port + 1); // lol
            mFrontend.SetHandler(new ClientCommandHandler(this));

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
            lock (mLock)
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

                for (int i = 0; i < MaxConnectionAttempts; i++)
                {
                    Thread.Sleep(ConnectionInterval);
                    if (mActiveProcess != null)
                    {
                        Log.Info($"Successfully connected to {Address}:{Port}!");
                        mDebuggeeRunning = true;
                        return true;
                    }

                    if (!mSession.IsRunning)
                    {
                        break;
                    }
                }

                Log.Error($"Could not connect to {Address}:{Port} - exiting");
                return false;
            }
        }

        private ExceptionInfo ActiveException => ActiveBacktrace?.GetFrame(0)?.GetException();
        private Backtrace ActiveBacktrace => ActiveThread?.Backtrace;

        private ThreadInfo ActiveThread
        {
            get
            {
                lock (mLock)
                {
                    return mSession?.ActiveThread;
                }
            }
        }

        private void OnStopped()
        {
            mVariableHandles.Clear();
            mFrameHandles.Clear();
            mCurrentException = null;
        }

        private void WaitForSuspend()
        {
            if (mDebuggeeRunning)
            {
                mResumeEvent.WaitOne();
                mDebuggeeRunning = false;
            }
        }

        public IPAddress Address { get; }
        public int Port { get; }

        private readonly ObjectRegistry<ObjectValue[]> mVariableHandles;
        private readonly ObjectRegistry<StackFrame> mFrameHandles;
        private ObjectValue mCurrentException;
        private readonly Dictionary<long, DebuggerThread> mSeenThreads;

        private bool mDisposed;
        private readonly object mLock;

        private readonly AutoResetEvent mResumeEvent;
        private bool mDebuggeeRunning;

        private ProcessInfo mActiveProcess;
        private StackFrame mActiveFrame;
        private long mNextBreakpointId;
        private readonly SortedDictionary<long, BreakEvent> mBreakpoints;
        private readonly List<Catchpoint> mCatchpoint;

        private readonly SoftDebuggerSession mSession;
        private readonly DebuggerFrontend mFrontend;
    }
}
