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
using System.Runtime.InteropServices;

namespace SGE
{
    /// <summary>
    /// An object representing four floating point numbers. Maps to glm::vec4.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct Vector4
    {
        public Vector4(float x, float y, float z, float w)
        {
            X = x;
            Y = y;
            Z = z;
            W = w;
        }

        public Vector4(Vector2 xy, float z, float w)
        {
            X = xy.X;
            Y = xy.Y;
            Z = z;
            W = w;
        }

        public Vector4(float x, Vector2 yz, float w)
        {
            X = x;
            Y = yz.X;
            Z = yz.Y;
            W = w;
        }

        public Vector4(float x, float y, Vector2 zw)
        {
            X = x;
            Y = y;
            Z = zw.X;
            W = zw.Y;
        }

        public Vector4(Vector3 xyz, float w)
        {
            X = xyz.X;
            Y = xyz.Y;
            Z = xyz.Z;
            W = w;
        }

        public Vector4(float x, Vector3 yzw)
        {
            X = x;
            Y = yzw.X;
            Z = yzw.Y;
            W = yzw.Z;
        }

        public Vector4(float scalar)
        {
            X = Y = Z = W = scalar;
        }

        public float X, Y, Z, W;

        public Vector2 XY => (X, Y);
        public Vector2 YZ => (Y, Z);
        public Vector2 ZW => (Z, W);
        public Vector2 XZ => (X, Z);
        public Vector2 XW => (X, W);
        public Vector2 YW => (Y, W);

        public float Length
        {
            get
            {
                float x2 = MathF.Pow(X, 2);
                float y2 = MathF.Pow(Y, 2);
                float z2 = MathF.Pow(Z, 2);
                float w2 = MathF.Pow(W, 2);
                return MathF.Sqrt(x2 + y2 + z2 + w2);
            }
        }

        public Vector4 Normalized => this / Length;

        public static Vector4 operator +(Vector4 lhs, Vector4 rhs) => (lhs.X + rhs.X, lhs.Y + rhs.Y, lhs.Z + rhs.Z, lhs.W + rhs.W);
        public static Vector4 operator +(Vector4 lhs, float rhs) => lhs + new Vector4(rhs);
        public static Vector4 operator -(Vector4 vector) => (-vector.X, -vector.Y, -vector.Z, -vector.W);
        public static Vector4 operator -(Vector4 lhs, Vector4 rhs) => lhs + -rhs;
        public static Vector4 operator -(Vector4 lhs, float rhs) => lhs + -rhs;
        public static Vector4 operator *(Vector4 lhs, Vector4 rhs) => (lhs.X * rhs.X, lhs.Y * rhs.Y, lhs.Z * rhs.Z, lhs.W * rhs.W);
        public static Vector4 operator *(Vector4 lhs, float rhs) => lhs * new Vector4(rhs);
        public static Vector4 operator /(Vector4 lhs, Vector4 rhs) => (lhs.X / rhs.X, lhs.Y / rhs.Y, lhs.Z / rhs.Z, lhs.W / rhs.W);
        public static Vector4 operator /(Vector4 lhs, float rhs) => lhs / new Vector4(rhs);

        public static implicit operator Vector4((float x, float y, float z, float w) tuple) => new Vector4(tuple.x, tuple.y, tuple.z, tuple.w);
    }
}
