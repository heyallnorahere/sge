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

using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Reflection;

namespace SGE.Debugger
{
    [AttributeUsage(AttributeTargets.Method, AllowMultiple = false, Inherited = false)]
    public sealed class CommandAttribute : Attribute
    {
        public CommandAttribute(string name)
        {
            Name = name;
        }

        public string Name { get; }
    }

    internal delegate dynamic HandlerCallback(dynamic args);
    internal sealed class HandlerCallbacks
    {
        public HandlerCallbacks()
        {
            mCallbacks = new Dictionary<string, HandlerCallback>();
        }

        public void Set(string name, HandlerCallback callback)
        {
            if (mCallbacks.ContainsKey(name))
            {
                mCallbacks[name] = callback;
            }
            else
            {
                mCallbacks.Add(name, callback);
            }
        }

        public void Call(string name, dynamic args)
        {
            if (!mCallbacks.ContainsKey(name))
            {
                return;
            }

            mCallbacks[name].Invoke(args);
        }

        private readonly Dictionary<string, HandlerCallback> mCallbacks;
    }

    public enum SocketMessageType
    {
        [JsonProperty(PropertyName = "request")]
        Request,

        [JsonProperty(PropertyName = "response")]
        Response,
    }

    public sealed class SocketMessage
    {
        [JsonProperty(PropertyName = "type")]
        public SocketMessageType Type { get; }

        [JsonProperty(PropertyName = "body")]
        public dynamic Body { get; }
    }

    public sealed class SocketRequest
    {
        public static SocketRequest Parse(dynamic body)
        {
            return new SocketRequest
            {
                Command = (string)body.command,
                Args = body.args
            };
        }

        private SocketRequest()
        {
            Command = string.Empty;
            Args = null;
        }

        public string Command { get; private set; }
        public dynamic Args { get; private set; }
    }

    internal sealed class StringBuffer
    {
        public StringBuffer()
        {
            mData = string.Empty;
        }

        public void Append(IEnumerable<char> data)
        {
            foreach (char character in data)
            {
                mData += character;
            }
        }

        public void Clear() => mData = string.Empty;
        public void Remove(int start, int end)
        {
            if (start < 0 || start >= end || end >= mData.Length)
            {
                throw new IndexOutOfRangeException();
            }

            mData = mData.Substring(0, start) + mData.Substring(end);
        }

        public string Data => mData;
        private string mData;
    }

    public sealed class DebuggerFrontend
    {
        private sealed class Connection : IDisposable
        {
            public static implicit operator Connection(Socket socket)
            {
                if (socket == null)
                {
                    return null;
                }

                return new Connection(socket);
            }

            private Connection(Socket socket)
            {
                mSocket = socket;
                mStream = new NetworkStream(mSocket);

                mReader = new StreamReader(mStream);
                mWriter = new StreamWriter(mStream);

                mDisposed = false;
            }

            public void Dispose()
            {
                if (mDisposed)
                {
                    return;
                }

                mWriter.Dispose();
                mReader.Dispose();
                mStream.Dispose();
                mSocket.Close();

                GC.SuppressFinalize(this);
                mDisposed = true;
            }

            public TextReader Reader => mReader;
            public TextWriter Writer => mWriter;

            private readonly Socket mSocket;
            private readonly Stream mStream;

            private readonly StreamReader mReader;
            private readonly StreamWriter mWriter;

            private bool mDisposed;
        }

        public DebuggerFrontend(int port)
        {
            mHandlers = new HandlerCallbacks();

            const string ipAddress = "127.0.0.1";
            mAddress = $"{ipAddress}:{port}";

            mLock = new object();
            mCurrentConnection = null;
            mServerRunning = false;
            mServer = new TcpListener(IPAddress.Parse(ipAddress), port);
        }

        public bool Run()
        {
            if (mServerRunning)
            {
                return false;
            }

            ListenForConnection();
            return true;
        }

        public bool Stop()
        {
            if (!mServerRunning)
            {
                return false;
            }

            mServerRunning = false;
            if (mCurrentConnection != null)
            {
                mCurrentConnection.Dispose();
            }

            mServer.Stop();
            return true;
        }

        private async void ListenForConnection()
        {
            mServerRunning = true;
            Log.Info($"Listening on {mAddress} for a client connection...");

            mServer.Start();
            while (mServerRunning)
            {
                mCurrentConnection = await mServer.AcceptSocketAsync();
                if (mCurrentConnection == null)
                {
                    continue;
                }

                Log.Info("Client connected! Listening for commands...");
                ListenForCommands();

                mCurrentConnection.Dispose();
                mCurrentConnection = null;
            }

            mServer.Stop();
        }

        private async void ListenForCommands()
        {
            const int bufferLength = 512;
            var tempBuffer = new char[bufferLength];

            var dataBuffer = new StringBuffer();
            while (true)
            {
                int lengthRead = await mCurrentConnection.Reader.ReadAsync(tempBuffer, 0, bufferLength);
                if (lengthRead == 0)
                {
                    break;
                }

                if (lengthRead < 0)
                {
                    continue;
                }

                var data = new string(tempBuffer, 0, lengthRead);
                dataBuffer.Append(data);

                ProcessData(dataBuffer);
            }
        }

        private void ProcessData(StringBuffer dataBuffer)
        {
            // todo: get buffer data
        }

        public void SetHandler<T>(T handler)
        {
            if (handler == null)
            {
                return;
            }

            var handlerType = handler.GetType();
            SetHandler(handlerType, BindingFlags.Instance, (type, method) => Delegate.CreateDelegate(type, handler, method));
        }

        public void SetHandler<T>() => SetHandler(typeof(T), BindingFlags.Static, Delegate.CreateDelegate);
        private void SetHandler(Type type, BindingFlags methodType, Func<Type, MethodInfo, Delegate> createDelegate)
        {
            lock (mLock)
            {
                var methods = type.GetMethods(methodType);
                foreach (var method in methods)
                {
                    var attribute = method.GetCustomAttribute<CommandAttribute>();
                    if (attribute == null)
                    {
                        continue;
                    }

                    try
                    {
                        var delegateType = typeof(HandlerCallback);
                        var delegateObject = (HandlerCallback)createDelegate(delegateType, method);

                        mHandlers.Set(attribute.Name, delegateObject);
                    }
                    catch (Exception)
                    {
                        continue;
                    }
                }
            }
        }

        private readonly HandlerCallbacks mHandlers;

        private bool mServerRunning;
        private Connection mCurrentConnection;
        private readonly TcpListener mServer;
        private readonly string mAddress;
        private readonly object mLock;
    }
}
