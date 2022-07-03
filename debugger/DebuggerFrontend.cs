﻿/*
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
using Newtonsoft.Json.Converters;
using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;

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

        public dynamic Call(string name, dynamic args)
        {
            if (!mCallbacks.ContainsKey(name))
            {
                return null;
            }

            return mCallbacks[name].Invoke(args);
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
        public SocketMessageType MessageType { get; set; }

        [JsonProperty(PropertyName = "body")]
        public dynamic Data { get; set; }
    }

    public sealed class SocketRequest
    {
        public static SocketRequest Parse(dynamic data)
        {
            return new SocketRequest
            {
                Command = (string)data.command,
                Args = data.args
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

    internal sealed class ByteBuffer
    {
        public ByteBuffer()
        {
            mData = new List<byte>();
        }

        public void Append(byte data) => mData.Add(data);
        public void Append(IEnumerable<byte> data) => mData.AddRange(data);

        public void Append(byte[] data, int count)
        {
            for (int i = 0; i < count; i++)
            {
                mData.Add(data[i]);
            }
        }

        public Action Clear => mData.Clear;
        public void Remove(int start, int end)
        {
            if (start < 0 || start >= end || end > mData.Count)
            {
                throw new IndexOutOfRangeException();
            }

            var data = mData.ToArray();
            mData.Clear();

            for (int i = 0; i < start; i++)
            {
                mData.Add(data[i]);
            }

            for (int i = end; i < mData.Count; i++)
            {
                mData.Add(data[i]);
            }
        }

        public string GetData(Encoding encoding) => encoding.GetString(mData.ToArray());
        private List<byte> mData;
    }

    public sealed class DebuggerFrontend
    {
        private sealed class Connection : IDisposable
        {
            public static implicit operator Connection(TcpClient client)
            {
                if (client == null)
                {
                    return null;
                }

                return new Connection(client);
            }

            private Connection(TcpClient client)
            {
                mClient = client;
                mStream = mClient.GetStream();

                mDisposed = false;
            }

            public void Dispose()
            {
                if (mDisposed)
                {
                    return;
                }

                mStream.Close();
                mClient.Close();

                GC.SuppressFinalize(this);
                mDisposed = true;
            }

            public Stream Stream => mStream;

            private readonly TcpClient mClient;
            private readonly NetworkStream mStream;

            private bool mDisposed;
        }

        private static readonly IReadOnlyDictionary<char, char> sScopeDeclarators;
        private static readonly JsonSerializerSettings sJsonSettings;

        private static void AddJsonConverter<T>() where T : JsonConverter, new() => sJsonSettings.Converters.Add(new T());
        static DebuggerFrontend()
        {
            sScopeDeclarators = new Dictionary<char, char>
            {
                ['{'] = '}',
                ['['] = ']',
                ['<'] = '>', // afaik won't be used, but better safe than sorry
            };

            sJsonSettings = new JsonSerializerSettings
            {
                MissingMemberHandling = MissingMemberHandling.Error,
                NullValueHandling = NullValueHandling.Include
            };

            AddJsonConverter<StringEnumConverter>();
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

        public void Run()
        {
            if (mServerRunning)
            {
                return;
            }

            mServerRunning = true;
            Log.Info($"Listening on {mAddress} for a client connection...");

            mServer.Start();
            while (mServerRunning)
            {
                if (!mServer.Pending())
                {
                    Thread.Sleep(1);
                    continue;
                }

                mCurrentConnection = mServer.AcceptTcpClient();
                if (mCurrentConnection == null)
                {
                    continue;
                }

                const int bufferLength = 512;
                var tempBuffer = new byte[bufferLength];

                Log.Info("Client connected - listening for commands...");
                var dataBuffer = new ByteBuffer();
                while (true)
                {
                    int lengthRead = mCurrentConnection.Stream.Read(tempBuffer, 0, bufferLength);
                    if (lengthRead == 0)
                    {
                        break;
                    }

                    if (lengthRead < 0)
                    {
                        continue;
                    }

                    dataBuffer.Append(tempBuffer, lengthRead);
                    ProcessData(dataBuffer);
                }

                mCurrentConnection.Dispose();
                mCurrentConnection = null;
                Log.Info("Client disconnected");
            }

            Log.Info("Stopped listening for connections");
            mServer.Stop();
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

        private int GetDataLength(string data)
        {
            var scopes = new Stack<char>();
            for (int i = 0; i < data.Length; i++)
            {
                char character = data[i];
                if (sScopeDeclarators.ContainsKey(character))
                {
                    scopes.Push(character);
                }
                else if (sScopeDeclarators.ContainsValue(character))
                {
                    if (scopes.Count == 0 || sScopeDeclarators[scopes.Peek()] != character)
                    {
                        throw new ArgumentException("Malformed JSON!");
                    }

                    scopes.Pop();
                }

                if (scopes.Count == 0)
                {
                    if (i > 0)
                    {
                        return i + 1;
                    }
                    else
                    {
                        throw new ArgumentException("Malformed JSON!");
                    }
                }
            }

            return -1;
        }

        private void ProcessData(ByteBuffer buffer)
        {
            try
            {
                string data = buffer.GetData(Encoding.ASCII);
                int dataLength = GetDataLength(data);

                if (dataLength < 0)
                {
                    return;
                }

                buffer.Remove(0, dataLength - 1);
                var message = JsonConvert.DeserializeObject<SocketMessage>(data);

                switch (message.MessageType)
                {
                    case SocketMessageType.Request:
                        DispatchRequest(message.Data);
                        break;
                    case SocketMessageType.Response:
                        // todo: dispatch
                        break;
                }
            }
            catch (Exception exc)
            {
                Log.Warn($"Could not parse received data: {exc.Message}");
                buffer.Clear();
            }
        }

        private void DispatchRequest(dynamic data)
        {
            SocketRequest request = SocketRequest.Parse(data);
            
            dynamic response;
            try
            {
                lock (mLock)
                {
                    response = mHandlers.Call(request.Command, request.Args);
                }
            }
            catch (Exception exc)
            {
                response = new
                {
                    exception = exc.GetType().FullName,
                    message = exc.Message
                };
            }

            SendResponse(response);
        }

        private void SendResponse(dynamic data)
        {
            var response = new SocketMessage
            {
                MessageType = SocketMessageType.Response,
                Data = data
            };

            var json = JsonConvert.SerializeObject(response, sJsonSettings);
            var bytes = Encoding.ASCII.GetBytes(json);
            mCurrentConnection.Stream.Write(bytes, 0, bytes.Length);
        }

        public void SetHandler(EventHandler handler)
        {
            if (handler == null)
            {
                return;
            }

            lock (mLock)
            {
                handler.EnumerateEventMethods<CommandAttribute>((method, attribute) =>
                {
                    try
                    {
                        var delegateType = typeof(HandlerCallback);
                        var delegateObject = (HandlerCallback)Delegate.CreateDelegate(delegateType, handler, method);

                        mHandlers.Set(attribute.Name, delegateObject);
                    }
                    catch (Exception)
                    {
                        // fall through
                    }
                });
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
