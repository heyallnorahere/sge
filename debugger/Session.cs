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
using System.IO;
using System.Linq;
using System.Net;
using System.Reflection;
using System.Text;
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

        protected abstract void HandleEventException(Exception exc, MethodBase method);
        private MethodBase GetEventMethod()
        {
            var stackFrame = new System.Diagnostics.StackFrame(2);
            return stackFrame.GetMethod();
        }

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
                    var method = GetEventMethod();
                    HandleEventException(exc, method);
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
                    var method = GetEventMethod();
                    HandleEventException(exc, method);

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

    internal struct ClientSettings
    {
        public void Parse(dynamic args)
        {
            UseURI = (bool)args.useUri;
            LineStart = (int)args.lineStart;
        }

        public bool UseURI { get; set; }
        public int LineStart { get; set; }
    }

    internal enum EvaluationErrorType
    {
        NoExpressionProvided,
        InvalidExpression,
        NotAvailable,
        NoActiveStackFrame
    }

    internal interface IEvaluationError
    {
        public string EvaluationError { get; }
    }

    internal sealed class StringEvaluationError : IEvaluationError
    {
        public StringEvaluationError(string error)
        {
            EvaluationError = error;
        }

        public string EvaluationError { get; }
    }

    internal sealed class EnumEvaluationError : IEvaluationError
    {
        public EnumEvaluationError(EvaluationErrorType type)
        {
            mType = type;
        }

        public string EvaluationError
        {
            get
            {
                string result = string.Empty;
                string typeName = mType.ToString();

                foreach (char character in typeName)
                {
                    if (char.IsUpper(character))
                    {
                        if (result.Length > 0)
                        {
                            result += ' ';
                        }

                        result += char.ToLower(character);
                    }
                    else
                    {
                        result += character;
                    }
                }

                return result;
            }
        }

        private readonly EvaluationErrorType mType;
    }

    public sealed class Session : IDisposable
    {
        private sealed class DebuggerEventHandler : EventHandler
        {
            public DebuggerEventHandler(Session session)
            {
                mSession = session;
            }

            private bool SendEvent(SocketEventType type, dynamic context)
            {
                return mSession.SendEvent(type, context);
            }

            protected override void HandleEventException(Exception exc, MethodBase method)
            {
                string excName = exc.GetType().FullName;
                Log.Error($"{excName} caught handling event {method.Name}: {exc.Message}");
            }

            [DebuggerEvent]
            private void TargetStopped(object sender, TargetEventArgs args) => HandleEvent(() =>
            {
                mSession.OnStopped();
                SendEvent(SocketEventType.DebuggerStep, new
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
                SendEvent(SocketEventType.BreakpointHit, new
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
                    SendEvent(eventType, new
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
                Log.Info("A handled exception was thrown");
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
                lock (mSession.mSeenThreads)
                {
                    var thread = new DebuggerThread(args.Thread);
                    mSession.mSeenThreads.Add(threadId, thread);
                }

                SendEvent(SocketEventType.ThreadStarted, threadId);
                Log.Info($"Target started a thread: {args.Thread.Name}");
            });

            [DebuggerEvent]
            private void TargetThreadStopped(object sender, TargetEventArgs args) => HandleEvent(() =>
            {
                long threadId = args.Thread.Id;
                string threadName = args.Thread.Name;

                lock (mSession.mSeenThreads)
                {
                    mSession.mSeenThreads.Remove(threadId);
                }

                Log.Info($"Target stopped a thread: {threadName}");
                SendEvent(SocketEventType.ThreadExited, new
                {
                    id = threadId,
                    name = threadName
                });
            });

            private readonly Session mSession;
        }

        private sealed class ClientCommandHandler : EventHandler
        {
            private static readonly HashSet<string> sMonoExtensions;
            static ClientCommandHandler()
            {
                sMonoExtensions = new HashSet<string>
                {
                    "cs", "csx", // C#
                    "cake", "hx", // i have no idea
                    "fs", "fsi", "ml", "mli", "fsx", "fsscript", // F#
                    "vb", // Visual Basic .NET
                };
            }

            private static bool HasMonoExtension(string path)
            {
                if (string.IsNullOrEmpty(path))
                {
                    return false;
                }

                int lastSeparatorIndex = path.LastIndexOf('.');
                if (lastSeparatorIndex < 0)
                {
                    return false;
                }

                string extension = path.Substring(lastSeparatorIndex + 1);
                return sMonoExtensions.Contains(extension);
            }

            public ClientCommandHandler(Session session)
            {
                mSession = session;
                mSettings = new ClientSettings
                {
                    UseURI = false,
                    LineStart = 1
                };
            }

            public const int DebuggerLineStart = 1;
            public int LineStartDifference => DebuggerLineStart - mSettings.LineStart;

            private int DebuggerLineToClient(int line) => line - LineStartDifference;
            private int ClientLineToDebugger(int line) => line + LineStartDifference;

            private string DebuggerPathToClient(string path)
            {
                if (mSettings.UseURI)
                {
                    try
                    {
                        var uri = new Uri(path);
                        return uri.AbsoluteUri;
                    }
                    catch (Exception)
                    {
                        return null;
                    }
                }
                else
                {
                    return Path.GetFullPath(path);
                }
            }

            private string ClientPathToDebugger(string path)
            {
                if (path == null)
                {
                    return null;
                }

                if (mSettings.UseURI)
                {
                    if (Uri.IsWellFormedUriString(path, UriKind.Absolute))
                    {
                        var uri = new Uri(path);
                        return uri.LocalPath;
                    }
                    else
                    {
                        throw new ArgumentException("Malformed URI!");
                    }
                }
                else
                {
                    return path;
                }
            }

            private void AddScope(List<DebuggerScope> scopes, string name, IEnumerable<ObjectValue> values, bool expensive = false)
            {
                int handle = mSession.mVariableHandles.Insert(values);
                var scope = new DebuggerScope(name, handle, expensive);
                scopes.Add(scope);
            }

            private void SubmitVariable(List<DebuggerVariable> variables, ObjectValue value)
            {
                var variable = DebuggerVariable.FromObjectValue(value, mSession.mVariableHandles);
                variables.Add(variable);
            }

            protected override void HandleEventException(Exception exc, MethodBase method)
            {
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

            [Command("setSettings")]
            private dynamic SetSettings(dynamic args) => HandleEvent(() =>
            {
                mSettings.Parse(args);
                return Null;
            });

            [Command("next")]
            private dynamic Next(dynamic args) => HandleEvent(() =>
            {
                mSession.WaitForSuspend();
                lock (mSession.mLock)
                {
                    if (!mSession.mSession.IsRunning && !mSession.mSession.HasExited)
                    {
                        mSession.mSession.NextLine();
                        mSession.mDebuggeeRunning = true;
                    }
                }

                return Null;
            });

            [Command("continue")]
            private dynamic Continue(dynamic args) => HandleEvent(() =>
            {
                mSession.WaitForSuspend();
                lock (mSession.mLock)
                {
                    if (!mSession.mSession.IsRunning && !mSession.mSession.HasExited)
                    {
                        mSession.mSession.Continue();
                        mSession.mDebuggeeRunning = true;
                    }
                }

                return Null;
            });

            [Command("stepIn")]
            private dynamic StepIn(dynamic args) => HandleEvent(() =>
            {
                mSession.WaitForSuspend();
                lock (mSession.mLock)
                {
                    if (!mSession.mSession.IsRunning && !mSession.mSession.HasExited)
                    {
                        mSession.mSession.StepLine();
                        mSession.mDebuggeeRunning = true;
                    }
                }

                return Null;
            });

            [Command("stepOut")]
            private dynamic StepOut(dynamic args) => HandleEvent(() =>
            {
                mSession.WaitForSuspend();
                lock (mSession.mLock)
                {
                    if (!mSession.mSession.IsRunning && !mSession.mSession.HasExited)
                    {
                        mSession.mSession.Finish();
                        mSession.mDebuggeeRunning = true;
                    }
                }

                return Null;
            });

            [Command("pause")]
            private dynamic Pause(dynamic args) => HandleEvent(() =>
            {
                lock (mSession.mLock)
                {
                    if (mSession.mSession.IsRunning)
                    {
                        mSession.mSession.Stop();
                    }
                }

                return Null;
            });

            [Command("stackTrace")]
            private dynamic StackTrace(dynamic args) => HandleEvent(() =>
            {
                int maxLevels = Utilities.GetValue<int>(args, "levels", 10);
                long threadId = Utilities.GetValue<long>(args, "threadId", 0);

                mSession.WaitForSuspend();

                var thread = mSession.ActiveThread;
                if (thread.Id != threadId)
                {
                    thread = mSession.FindThread(threadId);
                    if (thread != null)
                    {
                        thread.SetActive();
                    }
                }

                var stackFrames = new List<DebuggerStackFrame>();
                int totalFrames = 0;

                var backtrace = thread.Backtrace;
                if (backtrace != null && backtrace.FrameCount >= 0)
                {
                    totalFrames = backtrace.FrameCount;

                    for (int i = 0; i < Math.Min(totalFrames, maxLevels); i++)
                    {
                        var frame = backtrace.GetFrame(i);
                        string path = frame.SourceLocation.FileName;

                        // vscode specific i think, but i'm not gonna question it lol
                        string hint = "subtle";

                        DebuggerSource source = null;
                        if (!string.IsNullOrEmpty(path))
                        {
                            string sourceName = Path.GetFileName(path);
                            if (File.Exists(path))
                            {
                                hint = "normal";
                                string sourcePath = DebuggerPathToClient(path);
                                source = new DebuggerSource(sourceName, sourcePath, 0, hint);
                            }
                            else
                            {
                                source = new DebuggerSource(sourceName, null, 1000, "deemphasize");
                            }
                        }

                        string name = frame.SourceLocation.MethodName;
                        int line = DebuggerLineToClient(frame.SourceLocation.Line);
                        int handle = mSession.mFrameHandles.Insert(frame);

                        var sentFrame = new DebuggerStackFrame(handle, name, source, line, 0, hint);
                        stackFrames.Add(sentFrame);
                    }
                }

                return new
                {
                    stackFrames = stackFrames.ToArray(),
                    totalFrameCount = totalFrames
                };
            });

            [Command("scopes")]
            private dynamic Scopes(dynamic args) => HandleEvent(() =>
            {
                int frameId = Utilities.GetValue(args, "frameId", 0);
                var frame = mSession.mFrameHandles.Get(frameId);

                var scopes = new List<DebuggerScope>();
                if (frame.Index == 0 && mSession.mCurrentException != null)
                {
                    AddScope(scopes, "Exception", new ObjectValue[] { mSession.mCurrentException });
                }

                var localValues = new List<ObjectValue>
                {
                    frame.GetThisReference()
                };

                localValues.AddRange(frame.GetParameters());
                localValues.AddRange(frame.GetLocalVariables());

                ObjectValue[] locals = localValues.Where(x => x != null).ToArray();
                if (locals.Length > 0)
                {
                    AddScope(scopes, "Local", locals);
                }

                return scopes.ToArray();
            });

            [Command("variables")]
            private dynamic Variables(dynamic args) => HandleEvent(() =>
            {
                const int maxChildren = 100;

                int setId = Utilities.GetValue<int>(args, "variableSetId", 0);
                int offset = Utilities.GetValue<int>(args, "variableOffset", 0);

                mSession.WaitForSuspend();
                var variables = new List<DebuggerVariable>();
                bool moreVariablesExist = false;

                IEnumerable<ObjectValue> variableValues;
                if (mSession.mVariableHandles.TryGet(setId, out variableValues) &&
                    variableValues != null)
                {
                    var variableArray = variableValues.Skip(offset).ToArray();
                    if (variableArray.Length > 0)
                    {
                        if (variableArray.Length > maxChildren)
                        {
                            variableArray = variableArray.Take(maxChildren).ToArray();
                            moreVariablesExist = true;
                        }

                        if (variableArray.Length > 20)
                        {
                            foreach (var value in variableArray)
                            {
                                value.WaitHandle.WaitOne();
                                SubmitVariable(variables, value);
                            }
                        }
                        else
                        {
                            WaitHandle.WaitAll(variableArray.Select(x => x.WaitHandle).ToArray());
                            foreach (var value in variableArray)
                            {
                                SubmitVariable(variables, value);
                            }
                        }
                    }
                }

                return new
                {
                    variables = variables.ToArray(),
                    moreExist = moreVariablesExist
                };
            });

            [Command("threads")]
            private dynamic Threads(dynamic args) => HandleEvent(() =>
            {
                DebuggerThread[] threads = null;

                var process = mSession.mActiveProcess;
                if (process != null)
                {
                    Dictionary<long, DebuggerThread> threadTable;
                    lock (mSession.mSeenThreads)
                    {
                        threadTable = new Dictionary<long, DebuggerThread>(mSession.mSeenThreads);
                    }

                    var processThreads = process.GetThreads();
                    foreach (var thread in processThreads)
                    {
                        if (threadTable.ContainsKey(thread.Id))
                        {
                            continue;
                        }

                        threadTable.Add(thread.Id, new DebuggerThread(thread));
                    }

                    threads = threadTable.Values.ToArray();
                }

                return threads;
            });

            [Command("setBreakpoints")]
            private dynamic SetBreakpoints(dynamic args) => HandleEvent(() =>
            {
                string path = null;
                if (args.source != null)
                {
                    string sourcePath = (string)args.source.path;
                    if (sourcePath != null)
                    {
                        path = ClientPathToDebugger(sourcePath.Trim());
                    }
                }

                var breakpointsSet = Array.Empty<DebuggerBreakpoint>();
                if (HasMonoExtension(path))
                {
                    int[] clientLines = args.lines.ToObject<int[]>();
                    int[] debuggerLines = Array.ConvertAll(clientLines, ClientLineToDebugger);

                    var receievedBreakpoints = new HashSet<int>(debuggerLines);
                    var existingBreakpoints = new List<Tuple<long, int>>();

                    foreach (long id in mSession.mBreakpoints.Keys)
                    {
                        var breakpoint = mSession.mBreakpoints[id] as Breakpoint;
                        if (breakpoint != null && breakpoint.FileName == path)
                        {
                            existingBreakpoints.Add(new Tuple<long, int>(id, breakpoint.Line));
                        }
                    }

                    var keptBreakpoints = new HashSet<int>();
                    foreach (var breakpoint in existingBreakpoints)
                    {
                        if (receievedBreakpoints.Contains(breakpoint.Item2))
                        {
                            keptBreakpoints.Add(breakpoint.Item2);
                        }
                        else
                        {
                            if (mSession.mBreakpoints.TryGetValue(breakpoint.Item1, out BreakEvent breakEvent))
                            {
                                mSession.mBreakpoints.Remove(breakpoint.Item1);
                                mSession.mSession.Breakpoints.Remove(breakEvent);
                            }

                            Log.Info($"Removed breakpoint {breakpoint.Item1} at {path}:{breakpoint.Item2}");
                        }
                    }

                    var breakpoints = new List<DebuggerBreakpoint>();
                    foreach (var line in receievedBreakpoints)
                    {
                        if (!keptBreakpoints.Contains(line))
                        {
                            var breakEvent = mSession.mSession.Breakpoints.Add(path, line);

                            long id = mSession.mNextBreakpointId++;
                            mSession.mBreakpoints.Add(id, breakEvent);

                            Log.Info($"Added breakpoint {id} at {path}:{line}");
                        }

                        var breakpoint = new DebuggerBreakpoint(true, DebuggerLineToClient(line));
                        breakpoints.Add(breakpoint);
                    }

                    breakpointsSet = breakpoints.ToArray();
                }

                return breakpointsSet;
            });

            [Command("evaluate")] // why does HandleEvent<dynamic> need to be specified?????
            private dynamic Evaluate(dynamic args) => HandleEvent<dynamic>(() =>
            {
                // stupid, i know
                IEvaluationError error = null;

                string expression = Utilities.GetValue<string>(args, "expression", null);
                if (expression == null)
                {
                    error = new EnumEvaluationError(EvaluationErrorType.NoExpressionProvided);
                }
                else
                {
                    int frameId = Utilities.GetValue<int>(args, "frameId", 0);
                    var frame = mSession.mFrameHandles.Get(frameId);

                    if (frame == null)
                    {
                        error = new EnumEvaluationError(EvaluationErrorType.NoActiveStackFrame);
                    }
                    else
                    {
                        if (!frame.ValidateExpression(expression))
                        {
                            error = new EnumEvaluationError(EvaluationErrorType.InvalidExpression);
                        }
                        else
                        {
                            var options = EvaluationOptions.DefaultOptions;
                            ObjectValue value = frame.GetExpressionValue(expression, options);

                            value.WaitHandle.WaitOne();
                            var flags = value.Flags;

                            if (flags.HasFlags(ObjectValueFlags.Error) ||
                                flags.HasFlags(ObjectValueFlags.NotSupported))
                            {
                                string displayValue = value.DisplayValue;
                                if (displayValue.IndexOf("reference not available in the current evaluation context") > 0)
                                {
                                    error = new EnumEvaluationError(EvaluationErrorType.InvalidExpression);
                                }
                                else
                                {
                                    error = new StringEvaluationError(displayValue);
                                }
                            }
                            else if (flags.HasFlags(ObjectValueFlags.Unknown))
                            {
                                error = new EnumEvaluationError(EvaluationErrorType.InvalidExpression);
                            }
                            else if (flags.HasFlags(ObjectValueFlags.Object | ObjectValueFlags.Namespace))
                            {
                                error = new EnumEvaluationError(EvaluationErrorType.NotAvailable);
                            }
                            else
                            {
                                int handle = 0;
                                if (value.HasChildren)
                                {
                                    var children = value.GetAllChildren();
                                    handle = mSession.mVariableHandles.Insert(children);
                                }

                                return new
                                {
                                    error = (object)null,
                                    value = value.DisplayValue,
                                    childrenSetId = handle
                                };
                            }
                        }
                    }
                }

                return new
                {
                    error = error.EvaluationError,
                    value = (object)null,
                    childrenSetId = 0
                };
            });

            private readonly Session mSession;
            private ClientSettings mSettings;
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

            Log.Info($"Platform: {platform}, version {Environment.OSVersion.Version}, {Environment.ProcessorCount} processors");
            Log.Info($"Running on CLR version {Environment.Version}");

            string ip = args.Get("address");
            int port = int.Parse(args.Get("port"));

            using var session = new Session(ip, port);
            return session.Run();
        }

        private enum OutputSource
        {
            Debugger,
            Debuggee
        }

        private void WriteOutput(string text, bool isStdErr, OutputSource source)
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

            var eventType = source switch
            {
                OutputSource.Debugger => SocketEventType.DebuggerOutput,
                OutputSource.Debuggee => SocketEventType.DebuggeeOutput,
                _ => throw new ArgumentException("Invalid output source!")
            };

            SendEvent(eventType, new
            {
                text
            });
        }

        private void LogWriter(bool isStdErr, string text)
        {
            WriteOutput(text, isStdErr, OutputSource.Debugger);
        }

        private void OutputWriter(bool isStdErr, string text)
        {
            WriteOutput(text, isStdErr, OutputSource.Debuggee);
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

            mVariableHandles = new ObjectRegistry<IEnumerable<ObjectValue>>();
            mFrameHandles = new ObjectRegistry<StackFrame>();
            mCurrentException = null;
            mSeenThreads = new Dictionary<long, DebuggerThread>();

            mDisposed = false;
            mLock = new object();

            mResumeEvent = new AutoResetEvent(false);
            mDebuggeeRunning = false;

            mActiveProcess = null;
            mNextBreakpointId = 0;
            mBreakpoints = new SortedDictionary<long, BreakEvent>();

            mFrontend = new DebuggerFrontend(Port + 1, Encoding.UTF8);
            mFrontend.SetHandler(new ClientCommandHandler(this));

            mSession = new SoftDebuggerSession
            {
                Breakpoints = new BreakpointStore(),
                ExceptionHandler = exc => true,
                LogWriter = LogWriter,
                OutputWriter = OutputWriter
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

        private ThreadInfo FindThread(long threadId)
        {
            if (mSeenThreads.ContainsKey(threadId))
            {
                return mSeenThreads[threadId].Thread;
            }

            if (mActiveProcess != null)
            {
                var threads = mActiveProcess.GetThreads();
                foreach (var thread in threads)
                {
                    if (thread.Id == threadId)
                    {
                        return thread;
                    }
                }
            }

            return null;
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

        private bool SendEvent(SocketEventType type, dynamic context)
        {
            return mFrontend.SendEvent(type, context);
        }

        public IPAddress Address { get; }
        public int Port { get; }

        private readonly ObjectRegistry<IEnumerable<ObjectValue>> mVariableHandles;
        private readonly ObjectRegistry<StackFrame> mFrameHandles;
        private ObjectValue mCurrentException;
        private readonly Dictionary<long, DebuggerThread> mSeenThreads;

        private bool mDisposed;
        private readonly object mLock;

        private readonly AutoResetEvent mResumeEvent;
        private bool mDebuggeeRunning;

        private ProcessInfo mActiveProcess;
        private long mNextBreakpointId;
        private readonly SortedDictionary<long, BreakEvent> mBreakpoints;

        private readonly SoftDebuggerSession mSession;
        private readonly DebuggerFrontend mFrontend;
    }
}
