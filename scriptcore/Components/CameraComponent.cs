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

namespace SGE.Components
{
    public enum ProjectionType
    {
        Orthographic = 0,
        Perspective
    }
    public struct CameraClips
    {
        public float Near, Far;
    }

    /// <summary>
    /// A camera component adds a camera to the scene. The first primary camera component will render to the viewport.
    /// </summary>
    public sealed class CameraComponent
    {
        internal CameraComponent(IntPtr address)
        {
            mAddress = address;
        }

        public void SetOrthographic(float viewSize, CameraClips clips) => InternalCalls.SetOrthographic(mAddress, viewSize, clips);
        public void SetPerspective(float fov, CameraClips clips) => InternalCalls.SetPerspective(mAddress, fov, clips);

        public bool Primary
        {
            get => InternalCalls.GetPrimary(mAddress);
            set => InternalCalls.SetPrimary(mAddress, value);
        }

        public ProjectionType ProjectionType
        {
            get => InternalCalls.GetProjectionType(mAddress);
            set => InternalCalls.SetProjectionType(mAddress, value);
        }

        public float ViewSize
        {
            get => InternalCalls.GetViewSize(mAddress);
            set => InternalCalls.SetViewSize(mAddress, value);
        }

        public float FOV
        {
            get => InternalCalls.GetFOV(mAddress);
            set => InternalCalls.SetFOV(mAddress, value);
        }

        public CameraClips OrthographicClips
        {
            get
            {
                CameraClips clips;
                InternalCalls.GetOrthographicClips(mAddress, out clips);
                return clips;
            }
            set => InternalCalls.SetOrthographicClips(mAddress, value);
        }

        public CameraClips PerspectiveClips
        {
            get
            {
                CameraClips clips;
                InternalCalls.GetPerspectiveClips(mAddress, out clips);
                return clips;
            }
            set => InternalCalls.SetPerspectiveClips(mAddress, value);
        }

        private readonly IntPtr mAddress;
    }
}