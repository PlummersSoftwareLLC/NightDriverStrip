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
// History:     Jun-25-2022         Davepl      Based on Aurora
//
//---------------------------------------------------------------------------

/*
 * Aurora: https://github.com/pixelmatix/aurora
 * Copyright (c) 2014 Jason Coon
 *
 * Portions of this code are adapted from Andrew: http://pastebin.com/f22bfe94d
 * which, in turn, was "Adapted from the Life example on the Processing.org site"
 *
 * Made much more colorful by J.B. Langston:
 *  https://github.com/jblang/aurora/commit/6db5a884e3df5d686445c4f6b669f1668841929b
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

#ifndef PatternCube_H
#define PatternCube_H

#include "Geometry.h"

// Description:
// This file defines the PatternCube class, a subclass of LEDStripEffect.
// The class implements a 3D rotating cube effect on an LED matrix. It 
// features customizable parameters for cube dimensions, rotation angles, 
// focal length of the camera, and positioning. The cube is constructed, 
// rotated, and projected onto a 2D plane for display.
//
// Key Features:
// - 3D Cube rendering with adjustable size and rotation speed.
// - Camera perspective modeling with focal length and distance settings.
// - Efficient calculation of cube vertices, edges, and face visibility.
// - Dynamic LED color patterns based on cube's orientation and position.
//
// On displays that are 2X as wide as tall, two cubes will be drawn

class PatternCube : public LEDStripEffect
{
  private:
    float focal = 30; // Focal of the camera
    int cubeWidth = 28; // Cube size
    float Angx = 20.0, AngxSpeed = 0.05; // rotation (angle+speed) around X-axis
    float Angy = 10.0, AngySpeed = 0.05; // rotation (angle+speed) around Y-axis
    float Ox = 15.5, Oy = 15.5; // position (x,y) of the frame center
    int zCamera = 110; // distance from cube to the eye of the camera

    // Local vertices
    Vertex  local[8];
    // Camera aligned vertices
    Vertex  aligned[8];
    // On-screen projected vertices
    Point   screen[8];
    // Faces
    squareFace face[6];
    // Edges
    EdgePoint edge[12];
    int nbEdges;
    // ModelView matrix
    float m00, m01, m02, m10, m11, m12, m20, m21, m22;

    // constructs the cube
    void make(int w)
    {
      nbEdges = 0;

      local[0].set(-w, w, w);
      local[1].set(w, w, w);
      local[2].set(w, -w, w);
      local[3].set(-w, -w, w);
      local[4].set(-w, w, -w);
      local[5].set(w, w, -w);
      local[6].set(w, -w, -w);
      local[7].set(-w, -w, -w);

      face[0].set(1, 0, 3, 2);
      face[1].set(0, 4, 7, 3);
      face[2].set(4, 0, 1, 5);
      face[3].set(4, 5, 6, 7);
      face[4].set(1, 2, 6, 5);
      face[5].set(2, 3, 7, 6);

      int f, i;
      for (f = 0; f < 6; f++)
      {
        for (i = 0; i < face[f].length; i++)
        {
          face[f].ed[i] = this->findEdge(face[f].sommets[i], face[f].sommets[i ? i - 1 : face[f].length - 1]);
        }
      }
    }

    // finds edges from faces
    int findEdge(int a, int b)
    {
      int i;
      for (i = 0; i < nbEdges; i++)
        if ((edge[i].x == a && edge[i].y == b) || (edge[i].x == b && edge[i].y == a))
          return i;
      edge[nbEdges++].set(a, b);
      return i;
    }

    // rotates according to angle x&y
    void rotate(float angx, float angy)
    {
      int i;
      float cx = cos(angx);
      float sx = sin(angx);
      float cy = cos(angy);
      float sy = sin(angy);

      m00 = cy;
      m01 = 0;
      m02 = -sy;
      m10 = sx * sy;
      m11 = cx;
      m12 = sx * cy;
      m20 = cx * sy;
      m21 = -sx;
      m22 = cx * cy;

      for (i = 0; i < 8; i++)
      {
        aligned[i].x = m00 * local[i].x + m01 * local[i].y + m02 * local[i].z;
        aligned[i].y = m10 * local[i].x + m11 * local[i].y + m12 * local[i].z;
        aligned[i].z = m20 * local[i].x + m21 * local[i].y + m22 * local[i].z + zCamera;

        screen[i].x = floor((Ox + focal * aligned[i].x / aligned[i].z));
        screen[i].y = floor((Oy - focal * aligned[i].y / aligned[i].z));
      }

      for (i = 0; i < 12; i++)
        edge[i].visible = false;

      Point *pa, *pb, *pc;
      for (i = 0; i < 6; i++)
      {
        pa = screen + face[i].sommets[0];
        pb = screen + face[i].sommets[1];
        pc = screen + face[i].sommets[2];

        boolean back = ((pb->x - pa->x) * (pc->y - pa->y) - (pb->y - pa->y) * (pc->x - pa->x)) < 0;
        if (!back)
        {
          int j;
          for (j = 0; j < 4; j++)
          {
            edge[face[i].ed[j]].visible = true;
          }
        }
      }
    }

    uint8_t hue = 0;
    int step = 0;

    virtual size_t DesiredFramesPerSecond() const override
    {
        return 40;
    }

    bool RequiresDoubleBuffering() const override
    {
        return false;
    }


    void construct()
    {
      make(cubeWidth);
    }

  public:
    PatternCube() : LEDStripEffect(EFFECT_MATRIX_CUBE, "Cubes")
    {
      construct();
    }

    PatternCube(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
    {
      construct();
    }

    void Draw() override
    {
      g()->Clear();
      zCamera = beatsin8(2, 100, 140);
      AngxSpeed = beatsin8(3, 3, 10) / 100.0f;
      AngySpeed = g()->beatcos8(5, 3, 10) / 100.0f;

      // Update values
      Angx += AngxSpeed;
      Angy += AngySpeed;
      if (Angx >= TWO_PI)
        Angx -= TWO_PI;
      if (Angy >= TWO_PI)
        Angy -= TWO_PI;

      rotate(Angx, Angy);

      // Draw cube
      int i;

      for (int xOffset = 0; xOffset < MATRIX_WIDTH; xOffset +=32)
      {
        CRGB color = g()->ColorFromCurrentPalette(hue + 64 + xOffset);
        // Backface
        EdgePoint *e;
        for (i = 0; i < 12; i++)
        {
          e = edge + i;
          if (!e->visible) 
              g()->BresenhamLine(screen[e->x].x+xOffset, screen[e->x].y, screen[e->y].x+xOffset, screen[e->y].y, color);
        }

        color = g()->ColorFromCurrentPalette(hue + 128 + xOffset);

        // Frontface
        for (i = 0; i < 12; i++)
        {
          e = edge + i;
          if (e->visible)
              g()->BresenhamLine(screen[e->x].x+xOffset, screen[e->x].y, screen[e->y].x+xOffset, screen[e->y].y, color);
        }

        step++;
        if (step == 8) {
          step = 0;
          hue++;
        }
      }
    }
};

#endif
