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
using System;
using System.Runtime.CompilerServices;

namespace SGE
{
    internal static class InternalCalls
    {
        // entity
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool HasComponent(Type componentType, uint entityID, IntPtr scene);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern object GetComponent(Type componentType, uint entityID, IntPtr scene);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern GUID GetGUID(uint entityID, IntPtr scene);

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
        public static extern void SetBodyType(IntPtr component, BodyType bodyType);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool GetFixedRotation(IntPtr component);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetFixedRotation(IntPtr component, bool fixedRotation);

        // box collider component
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void GetSize(IntPtr component, out Vector2 size);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetSize(IntPtr component, Vector2 size);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern float GetDensity(IntPtr component);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetDensity(IntPtr component, float density);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern float GetFriction(IntPtr component);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetFriction(IntPtr component, float friction);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern float GetRestitution(IntPtr component);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetRestitution(IntPtr component, float restitution);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern float GetRestitutionThreashold(IntPtr component);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetRestitutionThreashold(IntPtr component, float threashold);

        // logger
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void LogDebug(string message);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void LogInfo(string message);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void LogWarn(string message);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void LogError(string message);
    }
}