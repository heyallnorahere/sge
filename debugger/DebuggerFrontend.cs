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
using Newtonsoft.Json.Converters;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Reflection;
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

    public enum SocketMessageType
    {
        Request,
        Response,
        Event,
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

        [JsonProperty(PropertyName = "command")]
        public string Command { get; private set; }

        [JsonProperty(PropertyName = "args")]
        public dynamic Args { get; private set; }
    }

    [AttributeUsage(AttributeTargets.Field, AllowMultiple = false, Inherited = false)]
    internal sealed class EventCategoryAttribute : Attribute
    {
        public EventCategoryAttribute(SocketEventCategory category)
        {
            Category = category;
        }

        public SocketEventCategory Category { get; }
    }

    public enum SocketEventType
    {
        [EventCategory(SocketEventCategory.Stopped)]
        DebuggerStep,

        [EventCategory(SocketEventCategory.Stopped)]
        BreakpointHit,

        [EventCategory(SocketEventCategory.Stopped)]
        HandledExceptionThrown,

        [EventCategory(SocketEventCategory.Stopped)]
        UnhandledExceptionThrown,

        [EventCategory(SocketEventCategory.Thread)]
        ThreadStarted,

        [EventCategory(SocketEventCategory.Thread)]
        ThreadExited
    }

    public enum SocketEventCategory
    {
        Stopped,
        Thread
    }

    public sealed class SocketEvent
    {
        [JsonProperty(PropertyName = "type")]
        public SocketEventType EventType { get; set; }

        [JsonProperty(PropertyName = "category")]
        public SocketEventCategory EventCategory { get; set; }

        [JsonProperty(PropertyName = "context")]
        public dynamic Context { get; set; }
    }

    internal sealed class Connection : IDisposable
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

    internal sealed class CommandCallbacks
    {
        public delegate dynamic Callback(dynamic args);
        public CommandCallbacks()
        {
            mCallbacks = new Dictionary<string, Callback>();
        }

        public void Set(string name, Callback callback)
        {
            var callbackMethod = callback.Method;
            var logMessage = $"Command {name} = {callbackMethod.GetFullName()}";

            if (mCallbacks.ContainsKey(name))
            {
                callbackMethod = mCallbacks[name].Method;
                logMessage += $" (overrides {callbackMethod.GetFullName()})";

                mCallbacks[name] = callback;
            }
            else
            {
                mCallbacks.Add(name, callback);
            }

            Log.Info(logMessage);
        }

        public dynamic Call(string name, dynamic args)
        {
            if (!mCallbacks.ContainsKey(name))
            {
                Log.Info($"Command {name} doesn't exist - response will not be sent");
                return null;
            }

            Log.Info($"Calling command: {name}");
            return mCallbacks[name].Invoke(args);
        }

        private readonly Dictionary<string, Callback> mCallbacks;
    }

    public sealed class DebuggerFrontend
    {
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

            sJsonSettings.Converters.Add(new StringEnumConverter(new CamelCase(), false));
        }

        public DebuggerFrontend(int port)
        {
            mHandlers = new CommandCallbacks();

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

                try
                {
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
                }
                catch (IOException)
                {
                    // error occurred or server shut down, let's just silently close
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
            while (!mServerRunning)
            {
                Thread.Sleep(1);
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

                if (message.MessageType == SocketMessageType.Request)
                {
                    DispatchRequest(message.Data);
                }
                else
                {
                    throw new ArgumentException($"Invalid message type: {message.MessageType}!");
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

            if (response != null)
            {
                SendResponse(response);
            }
        }

        private void SendResponse(dynamic data)
        {
            var response = new SocketMessage
            {
                MessageType = SocketMessageType.Response,
                Data = data
            };

            SendMessage(response);
        }

        public bool SendEvent(SocketEventType type, dynamic context)
        {
            if (mCurrentConnection == null)
            {
                return false;
            }

            var enumType = typeof(SocketEventType);
            var members = enumType.GetMember(type.ToString());

            if (members.Length == 0)
            {
                return false;
            }

            var member = members.FirstOrDefault(m => m.DeclaringType == enumType);
            var attribute = member.GetCustomAttribute<EventCategoryAttribute>();

            if (attribute == null)
            {
                return false;
            }

            var eventMessage = new SocketMessage
            {
                MessageType = SocketMessageType.Event,
                Data = new SocketEvent
                {
                    EventType = type,
                    EventCategory = attribute.Category,
                    Context = context
                }
            };

            SendMessage(eventMessage);
            return true;
        }

        private void SendMessage(SocketMessage message)
        {
            var json = JsonConvert.SerializeObject(message, sJsonSettings);
            var bytes = Encoding.ASCII.GetBytes(json);

            var stream = mCurrentConnection.Stream;
            stream.Write(bytes, 0, bytes.Length);
            stream.Flush();
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
                        var delegateType = typeof(CommandCallbacks.Callback);
                        var delegateObject = (CommandCallbacks.Callback)Delegate.CreateDelegate(delegateType, handler, method);

                        mHandlers.Set(attribute.Name, delegateObject);
                    }
                    catch (Exception)
                    {
                        // fall through
                    }
                });
            }
        }

        private readonly CommandCallbacks mHandlers;
        private bool mServerRunning;
        private Connection mCurrentConnection;
        private readonly TcpListener mServer;
        private readonly string mAddress;
        private readonly object mLock;
    }
}
