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

using SGE.Components;
using SGE.Events;
using System;
using System.Runtime.CompilerServices;

namespace SGE
{
    /// <summary>
    /// Functions defined in sge/script/internal_calls.cpp
    /// </summary>
    internal static class InternalCalls
    {
        // scene
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern uint CreateEntity(string name, IntPtr scene);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern uint CreateEntityWithGUID(GUID id, string name, IntPtr scene);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern uint CloneEntity(uint entityID, string name, IntPtr scene);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void DestroyEntity(uint entityID, IntPtr scene);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool FindEntity(GUID id, out uint entityID, IntPtr scene);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern string GetCollisionCategoryName(IntPtr scene, int index);

        // entity
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern IntPtr AddComponent(Type componentType, Entity entity);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool HasComponent(Type componentType, Entity entity);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern IntPtr GetComponent(Type componentType, Entity entity);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern GUID GetGUID(Entity entity);

        // guid
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern GUID GenerateGUID();

        // tag component
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetTag(IntPtr component, string tag);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern string GetTag(IntPtr component);

        // transform component
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void GetTranslation(IntPtr component, out Vector2 translation);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetTranslation(IntPtr component, Vector2 translation);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern float GetRotation(IntPtr component);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetRotation(IntPtr component, float rotation);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void GetScale(IntPtr component, out Vector2 scale);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetScale(IntPtr component, Vector2 scale);

        // sprite renderer component
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void GetColor(IntPtr component, out Vector4 color);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetColor(IntPtr component, Vector4 color);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void GetTexture(IntPtr component, out IntPtr texture);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetTexture(IntPtr component, IntPtr texture);

        // camera component
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool GetPrimary(IntPtr component);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetPrimary(IntPtr component, bool primary);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern ProjectionType GetProjectionType(IntPtr component);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetProjectionType(IntPtr component, ProjectionType type);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern float GetViewSize(IntPtr component);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetViewSize(IntPtr component, float viewSize);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern float GetFOV(IntPtr component);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetFOV(IntPtr component, float fov);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void GetOrthographicClips(IntPtr component, out CameraClips clips);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetOrthographicClips(IntPtr component, CameraClips clips);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void GetPerspectiveClips(IntPtr component, out CameraClips clips);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetPerspectiveClips(IntPtr component, CameraClips clips);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetOrthographic(IntPtr component, float viewSize, CameraClips clips);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetPerspective(IntPtr component, float fov, CameraClips clips);

        // rigid body component
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern BodyType GetBodyType(IntPtr component);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetBodyType(IntPtr component, Entity entity, BodyType bodyType);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool GetFixedRotation(IntPtr component);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetFixedRotation(IntPtr component, Entity entity, bool fixedRotation);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool GetAngularVelocity(Entity entity, out float velocity);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool SetAngularVelocity(Entity entity, float velocity);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool AddForce(Entity entity, Vector2 force, bool wake);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern ushort GetFilterCategory(IntPtr component);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetFilterCategory(IntPtr component, ushort category);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern ushort GetFilterMask(IntPtr component);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetFilterMask(IntPtr component, ushort mask);

        // box collider component
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void GetSize(IntPtr component, out Vector2 size);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetSize(IntPtr component, Entity entity, Vector2 size);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern float GetDensity(IntPtr component);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetDensity(IntPtr component, Entity entity, float density);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern float GetFriction(IntPtr component);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetFriction(IntPtr component, Entity entity, float friction);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern float GetRestitution(IntPtr component);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetRestitution(IntPtr component, Entity entity, float restitution);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern float GetRestitutionThreashold(IntPtr component);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetRestitutionThreashold(IntPtr component, Entity entity, float threashold);

        // logger
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void LogDebug(string message);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void LogInfo(string message);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void LogWarn(string message);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void LogError(string message);

        // input
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool GetKey(KeyCode key);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool GetMouseButton(MouseButton button);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void GetMousePosition(out Vector2 position);

        // events
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool IsEventHandled(IntPtr address);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetEventHandled(IntPtr address, bool handled);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern int GetResizeWidth(IntPtr address);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern int GetResizeHeight(IntPtr address);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void GetPressedEventKey(IntPtr address, out KeyCode key);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern int GetRepeatCount(IntPtr address);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void GetReleasedEventKey(IntPtr address, out KeyCode key);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void GetTypedEventKey(IntPtr address, out KeyCode key);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void GetEventMousePosition(IntPtr address, out Vector2 position);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void GetScrollOffset(IntPtr address, out Vector2 offset);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void GetEventMouseButton(IntPtr address, out MouseButton button);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool GetMouseButtonReleased(IntPtr address);

        // texture2d
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void AddRef_texture_2d(IntPtr address);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void RemoveRef_texture_2d(IntPtr address);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void LoadTexture2D(string path, out IntPtr address);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern TextureWrap GetWrapTexture2D(IntPtr address);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern TextureFilter GetFilterTexture2D(IntPtr address);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern string GetPathTexture2D(IntPtr address);

        // script component
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool IsScriptEnabled(IntPtr address);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetScriptEnabled(IntPtr address, bool enabled);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern Script GetScript(IntPtr address, Entity entity);

        // file changed event
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern string GetChangedFilePath(IntPtr address);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern string GetWatchedDirectory(IntPtr address);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void GetFileStatus(IntPtr address, out FileStatus status);
    }
}