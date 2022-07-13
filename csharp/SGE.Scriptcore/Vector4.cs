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

        public Vector2 XY => new Vector2(X, Y);
        public Vector2 YZ => new Vector2(Y, Z);
        public Vector2 ZW => new Vector2(Z, W);
        public Vector2 XZ => new Vector2(X, Z);
        public Vector2 XW => new Vector2(X, W);
        public Vector2 YW => new Vector2(Y, W);

        public float Length
        {
            get
            {
                double x2 = Math.Pow(X, 2);
                double y2 = Math.Pow(Y, 2);
                double z2 = Math.Pow(Z, 2);
                double w2 = Math.Pow(W, 2);
                return (float)Math.Sqrt(x2 + y2 + z2 + w2);
            }
        }

        public Vector4 Normalized => this / Length;

        public static Vector4 operator +(Vector4 lhs, Vector4 rhs) => new Vector4(lhs.X + rhs.X, lhs.Y + rhs.Y, lhs.Z + rhs.Z, lhs.W + rhs.W);
        public static Vector4 operator +(Vector4 lhs, float rhs) => lhs + new Vector4(rhs);
        public static Vector4 operator -(Vector4 vector) => new Vector4(-vector.X, -vector.Y, -vector.Z, -vector.W);
        public static Vector4 operator -(Vector4 lhs, Vector4 rhs) => lhs + -rhs;
        public static Vector4 operator -(Vector4 lhs, float rhs) => lhs + -rhs;
        public static Vector4 operator *(Vector4 lhs, Vector4 rhs) => new Vector4(lhs.X * rhs.X, lhs.Y * rhs.Y, lhs.Z * rhs.Z, lhs.W * rhs.W);
        public static Vector4 operator *(Vector4 lhs, float rhs) => lhs * new Vector4(rhs);
        public static Vector4 operator /(Vector4 lhs, Vector4 rhs) => new Vector4(lhs.X / rhs.X, lhs.Y / rhs.Y, lhs.Z / rhs.Z, lhs.W / rhs.W);
        public static Vector4 operator /(Vector4 lhs, float rhs) => lhs / new Vector4(rhs);
    }
}
