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
    /// An object representing two floating point numbers. Maps to glm::vec2.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct Vector2
    {
        public Vector2(float x, float y)
        {
            X = x;
            Y = y;
        }

        public Vector2(float scalar)
        {
            X = Y = scalar;
        }

        public float X, Y;

        public float Length
        {
            get
            {
                double x2 = Math.Pow(X, 2);
                double y2 = Math.Pow(Y, 2);
                return (float)Math.Sqrt(x2 + y2);
            }
        }

        public Vector2 Normalized => this / Length;

        public static Vector2 operator +(Vector2 lhs, Vector2 rhs) => new Vector2(lhs.X + rhs.X, lhs.Y + rhs.Y);
        public static Vector2 operator +(Vector2 lhs, float rhs) => lhs + new Vector2(rhs);
        public static Vector2 operator -(Vector2 vector) => new Vector2(-vector.X, -vector.Y);
        public static Vector2 operator -(Vector2 lhs, Vector2 rhs) => lhs + -rhs;
        public static Vector2 operator -(Vector2 lhs, float rhs) => lhs + -rhs;
        public static Vector2 operator *(Vector2 lhs, Vector2 rhs) => new Vector2(lhs.X * rhs.X, lhs.Y * rhs.Y);
        public static Vector2 operator *(Vector2 lhs, float rhs) => lhs * new Vector2(rhs);
        public static Vector2 operator /(Vector2 lhs, Vector2 rhs) => new Vector2(lhs.X / rhs.X, lhs.Y / rhs.Y);
        public static Vector2 operator /(Vector2 lhs, float rhs) => lhs / new Vector2(rhs);
    }
}
