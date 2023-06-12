//+--------------------------------------------------------------------------
//
// File:        PatternMaze.h
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
// History:     Jul-27-2022         Davepl      Based on Aurora
//
//---------------------------------------------------------------------------

/*
 * Aurora: https://github.com/pixelmatix/aurora
 * Copyright (c) 2014 Jason Coon
 *
 * Many thanks to Jamis Buck for the documentation of the Growing Tree maze generation algorithm: http://weblog.jamisbuck.org/2011/1/27/maze-generation-growing-tree-algorithm
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

#ifndef PatternMaze_H
#define PatternMaze_H

class PatternMaze : public LEDStripEffect 
{
private:
    enum Directions {
        None = 0,
        Up = 1,
        Down = 2,
        Left = 4,
        Right = 8,
    };

    struct Point{
        int x;
        int y;

        static Point New(int x, int y) {
            Point point;
            point.x = x;
            point.y = y;
            return point;
        }

        Point Move(Directions direction) {
            switch (direction)
            {
                case Up:
                    return New(x, y - 1);

                case Down:
                    return New(x, y + 1);

                case Left:
                    return New(x - 1, y);

                case Right:
                default:
                    return New(x + 1, y);
            }
        }

        static Directions Opposite(Directions direction) {
            switch (direction) {
                case Up:
                    return Down;

                case Down:
                    return Up;

                case Left:
                    return Right;

                case Right:
                default:
                    return Left;
            }
        }
    };

//    int width = 16;
//    int height = 16;

    static const int width = MATRIX_WIDTH / 4;
    static const int height = MATRIX_HEIGHT / 2;
    

    Directions grid[width][height];

    Point point;

    Point cells[width*height];
    int cellCount = 0;

    int algorithm = 0;
    int algorithmCount = 1;

    size_t hue = 0;

    Directions directions[4] = { Up, Down, Left, Right };

    void removeCell(int index) {// shift cells after index down one
        for (int i = index; i < cellCount - 1; i++) {
            cells[i] = cells[i + 1];
        }

        cellCount--;
    }

    void shuffleDirections() {
        for (int a = 0; a < 4; a++)
        {
            int r = random(a, 4);
            Directions temp = directions[a];
            directions[a] = directions[r];
            directions[r] = temp;
        }
    }

    Point createPoint(int x, int y) {
        Point point;
        point.x = x;
        point.y = y;
        return point;
    }

    CRGB chooseColor(int index) 
    {
        switch (algorithm) {
            case 0:
            default:
                return CHSV(hue, 255, 200);
            case 1:
                return CHSV(hue + 128, 255, 200);
        }
    }
  
    int chooseIndex(int max) 
    {
        switch (algorithm) {
            case 0:
            default:
                // choose newest (recursive backtracker)
                return max - 1;

            case 1:
                // choose random(Prim's)
                return random(max);

            //case 2:
                // choose oldest (not good, so disabling)
            //    return 0;
        }
    }

    void generateMaze() {
        while (cellCount > 1) {
            drawNextCell();
        }
    }

    void drawNextCell() 
    {
        int index = chooseIndex(cellCount);

        if (index < 0)
            return;

        point = cells[index];

        Point imagePoint = createPoint(point.x * 2, point.y * 2);

        shuffleDirections();

        CRGB color = chooseColor(index);
        g()->drawPixel(imagePoint.x, imagePoint.y, color);
        g()->drawPixel(MATRIX_WIDTH - 1 - imagePoint.x, imagePoint.y, color);

        for (int i = 0; i < 4; i++) 
        {
            Directions direction = directions[i];

            Point newPoint = point.Move(direction);
            if (newPoint.x >= 0 && newPoint.y >= 0 && newPoint.x < width && newPoint.y < height && grid[newPoint.y][newPoint.x] == None) 
            {
                grid[point.y][point.x]       = (Directions) ((int) grid[point.y][point.x] | (int) direction);
                grid[newPoint.y][newPoint.x] = (Directions) ((int) grid[newPoint.y][newPoint.x] | (int) point.Opposite(direction));

                Point newImagePoint = imagePoint.Move(direction);

                g()->drawPixel(newImagePoint.x, newImagePoint.y, color);
                g()->drawPixel(MATRIX_WIDTH - 1 - newImagePoint.x, newImagePoint.y, color);

                cellCount++;
                cells[cellCount - 1] = newPoint;

                index = -1;
                break;
            }
        }

        if (index > -1) {
            Point finishedPoint = cells[index];
            imagePoint = createPoint(finishedPoint.x * 2, finishedPoint.y * 2);
            g()->drawPixel(imagePoint.x, imagePoint.y, color);
            g()->drawPixel(MATRIX_WIDTH - 1 - imagePoint.x, imagePoint.y, color);

            removeCell(index);
        }
    }

public:

    PatternMaze() : LEDStripEffect(EFFECT_MATRIX_MAZE, "Maze")
    {
    }

    PatternMaze(const JsonObjectConst& jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    virtual void Draw() override
    {
        if (cellCount < 1) 
        {
            hue = random(256);
            g()->Clear();

            // reset the maze grid
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    grid[x][y] = None;
                }
            }

            int x = random(width);
            int y = random(height);

            cells[0] = createPoint(x, y);

            cellCount = 1;

        }

        drawNextCell();

        if (cellCount < 1) {
            algorithm++;
            if (algorithm >= algorithmCount)
                algorithm = 0;
        }
    }

    virtual bool Init(std::shared_ptr<GFXBase> gfx[NUM_CHANNELS])
    {
        if (!LEDStripEffect::Init(gfx))
            return false;

        cellCount = 0;
        hue = 0;
        return true;
    }

    virtual void Start() override
    {
        g()->Clear();
    }
};

#endif
