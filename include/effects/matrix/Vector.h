#pragma once

//+--------------------------------------------------------------------------
//
// File:        PatternSpiro.h
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
//
// This file is part of the NightDriver software project.
//
//    NightDriver is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    NightDriver is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Nightdriver.  It is normally found in copying.txt
//    If not, see <https://www.gnu.org/licenses/>.
//
//
// Description:
//
//   Effect code ported from Aurora to Mesmerizer's draw routines
//
// History:     Jul-08-2022         Davepl      From Aurora
//
//---------------------------------------------------------------------------

/*
 * Aurora: https://github.com/pixelmatix/aurora
 * Copyright (c) 2014 Jason Coon
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef Vector_H
#define Vector_H

template <class T>
class Vector2
{
public:
    T x, y;

    Vector2() : x(0), y(0) {}
    Vector2(T x, T y) : x(x), y(y) {}
    Vector2(const Vector2 &v) : x(v.x), y(v.y) {}

    Vector2 &operator=(const Vector2 &v)
    {
        x = v.x;
        y = v.y;
        return *this;
    }

    bool isEmpty() const
    {
        return x == 0 && y == 0;
    }

    bool operator==(const Vector2 &v) const
    {
        return x == v.x && y == v.y;
    }

    bool operator!=(const Vector2 &v) const
    {
        return !(x == y);
    }

    Vector2 operator+(const Vector2 &v) const
    {
        return Vector2(x + v.x, y + v.y);
    }
    
    Vector2 operator-(const Vector2 &v) const
    {
        return Vector2(x - v.x, y - v.y);
    }

    Vector2 &operator+=(const Vector2 &v)
    {
        x += v.x;
        y += v.y;
        return *this;
    }
    Vector2 &operator-=(const Vector2 &v)
    {
        x -= v.x;
        y -= v.y;
        return *this;
    }

    Vector2 operator+(float s) const
    {
        return Vector2(x + s, y + s);
    }
    Vector2 operator-(float s) const
    {
        return Vector2(x - s, y - s);
    }
    Vector2 operator*(float s) const
    {
        return Vector2(x * s, y * s);
    }
    Vector2 operator/(float s) const
    {
        return Vector2(x / s, y / s);
    }

    Vector2 &operator+=(float s)
    {
        x += s;
        y += s;
        return *this;
    }
    Vector2 &operator-=(float s)
    {
        x -= s;
        y -= s;
        return *this;
    }
    Vector2 &operator*=(float s)
    {
        x *= s;
        y *= s;
        return *this;
    }
    Vector2 &operator/=(float s)
    {
        x /= s;
        y /= s;
        return *this;
    }

    void set(T x, T y)
    {
        this->x = x;
        this->y = y;
    }

    void rotate(float deg)
    {
        float theta = deg / 180.0f * (float) M_PI;
        float c = cos(theta);
        float s = sin(theta);
        float tx = x * c - y * s;
        float ty = x * s + y * c;
        x = tx;
        y = ty;
    }

    Vector2 &normalize()
    {
        if (length() == 0)
            return *this;
        *this *= (1.0f / length());
        return *this;
    }

    float dist(Vector2 v) const
    {
        Vector2 d(v.x - x, v.y - y);
        return d.length();
    }
    float length() const
    {
        return sqrt(x * x + y * y);
    }

    float mag() const
    {
        return length();
    }

    float magSq() const
    {
        return (x * x + y * y);
    }

    void truncate(float length)
    {
        float angle = atan2f(y, x);
        x = length * cos(angle);
        y = length * sin(angle);
    }

    Vector2 ortho() const
    {
        return Vector2(y, -x);
    }

    static float dot(Vector2 v1, Vector2 v2) 
    {
        return v1.x * v2.x + v1.y * v2.y;
    }
    static float cross(Vector2 v1, Vector2 v2) 
    {
        return (v1.x * v2.y) - (v1.y * v2.x);
    }

    void limit(float max) 
    {
        if (magSq() > max * max)
        {
            normalize();
            *this *= max;
        }
    }
};

typedef Vector2<float> PVector;

#endif
