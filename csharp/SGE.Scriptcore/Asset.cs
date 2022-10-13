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
using System.Collections.Generic;
using System.Reflection;

namespace SGE
{
    public enum AssetType : int
    {
        Shader = 0,
        Texture2D,
        Prefab,
        Sound,
        Shape
    }

    [AttributeUsage(AttributeTargets.Class, AllowMultiple = false, Inherited = false)]
    internal sealed class TypedAssetAttribute : Attribute
    {
        public TypedAssetAttribute(AssetType type)
        {
            Type = type;
        }

        public AssetType Type { get; }
    }

    public abstract class Asset
    {
        static readonly IReadOnlyDictionary<AssetType, ConstructorInfo> sAssetConstructors;
        static Asset()
        {
            var assembly = Assembly.GetExecutingAssembly();
            var types = assembly.GetTypes();

            var constructors = new Dictionary<AssetType, ConstructorInfo>();
            foreach (var type in types)
            {
                var attribute = type.GetCustomAttribute<TypedAssetAttribute>();
                if (attribute != null)
                {
                    if (constructors.ContainsKey(attribute.Type))
                    {
                        Log.Error($"asset type already exists: {attribute.Type}");
                        continue;
                    }

                    var constructor = type.GetConstructor(BindingFlags.NonPublic | BindingFlags.Instance, null, new Type[] { typeof(IntPtr) }, null);
                    if (constructor == null)
                    {
                        Log.Error($"could not find a suitable constructor for {type.FullName}");
                        continue;
                    }

                    constructors.Add(attribute.Type, constructor);
                }
            }

            sAssetConstructors = constructors;
        }

        internal static Asset FromPointer(IntPtr address)
        {
            CoreInternalCalls.GetAssetType(address, out AssetType type);
            if (!sAssetConstructors.ContainsKey(type))
            {
                return null;
            }

            var constructor = sAssetConstructors[type];
            return (Asset)constructor.Invoke(new object[] { address });
        }

        internal Asset(IntPtr address)
        {
            mAddress = address;
        }

        public bool Reload() => CoreInternalCalls.ReloadAsset(mAddress);

        public string Path => CoreInternalCalls.GetAssetPath(mAddress);

        public AssetType Type
        {
            get
            {
                CoreInternalCalls.GetAssetType(mAddress, out AssetType type);
                return type;
            }
        }

        public GUID GUID
        {
            get
            {
                CoreInternalCalls.GetAssetGUID(mAddress, out GUID guid);
                return guid;
            }
        }


        internal readonly IntPtr mAddress;
    }
}
