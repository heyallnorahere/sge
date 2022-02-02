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
    /// An object representing three floating point numbers. Maps to glm::vec3.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct Vector3
    {
        public Vector3(float x, float y, float z)
        {
            X = x;
            Y = y;
            Z = z;
        }

        public Vector3(Vector2 xy, float z)
        {
            X = xy.X;
            Y = xy.Y;
            Z = z;
        }

        public Vector3(float x, Vector2 yz)
        {
            X = x;
            Y = yz.X;
            Z = yz.Y;
        }

        public Vector3(Vector4 vector)
        {
            X = vector.X;
            Y = vector.Y;
            Z = vector.Z;
        }

        public Vector3(float scalar)
        {
            X = Y = Z = scalar;
        }

        public float X, Y, Z;

        public Vector2 XY => new Vector2(X, Y);
        public Vector2 YZ => new Vector2(Y, Z);
        public Vector2 XZ => new Vector2(X, Z);

        public float Length
        {
            get
            {
                float x2 = MathF.Pow(X, 2);
                float y2 = MathF.Pow(Y, 2);
                float z2 = MathF.Pow(Z, 2);
                return MathF.Sqrt(x2 + y2 + z2);
            }
        }

        public Vector3 Normalized => this / Length;

        public static Vector3 operator +(Vector3 lhs, Vector3 rhs) => (lhs.X + rhs.X, lhs.Y + rhs.Y, lhs.Z + rhs.Z);
        public static Vector3 operator +(Vector3 lhs, float rhs) => lhs + new Vector3(rhs);
        public static Vector3 operator -(Vector3 vector) => (-vector.X, -vector.Y, -vector.Z);
        public static Vector3 operator -(Vector3 lhs, Vector3 rhs) => lhs + -rhs;
        public static Vector3 operator -(Vector3 lhs, float rhs) => lhs + -rhs;
        public static Vector3 operator *(Vector3 lhs, Vector3 rhs) => (lhs.X * rhs.X, lhs.Y * rhs.Y, lhs.Z * rhs.Z);
        public static Vector3 operator *(Vector3 lhs, float rhs) => lhs * new Vector3(rhs);
        public static Vector3 operator /(Vector3 lhs, Vector3 rhs) => (lhs.X / rhs.X, lhs.Y / rhs.Y, lhs.Z / rhs.Z);
        public static Vector3 operator /(Vector3 lhs, float rhs) => lhs / new Vector3(rhs);

        public static implicit operator Vector3((float x, float y, float z) tuple) => new Vector3(tuple.x, tuple.y, tuple.z);
    }
}
