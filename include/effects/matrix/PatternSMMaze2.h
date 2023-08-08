#pragma once

#include "effectmanager.h"

// Derived from https://editor.soulmatelights.com/gallery/1956-maze
// We have another maze generator, but this is self-solving.

class PatternSMMaze2 : public LEDStripEffect
{
  private:
    static constexpr int LED_COLS = MATRIX_WIDTH;
    static constexpr int LED_ROWS = MATRIX_HEIGHT;
#define M_HEIGHT LED_ROWS + !(LED_ROWS % 2)
#define M_WIDTH LED_COLS + !(LED_COLS % 2)
#define M_SHIFT_X !(LED_COLS % 2)
#define M_SHIFT_Y !(LED_ROWS % 2)
    bool maze[M_WIDTH][M_HEIGHT];
    bool start = true;
    byte posX, posY;
    byte color;
    byte Lookdir;
    bool checkFlag;
    bool tale = 0;
    int SubPos;

    CRGB colorsmear(const CRGB &col1, const CRGB &col2, byte l)
    {
        CRGB temp = col1;
        nblend(temp, col2, l);
        return temp;
    }

    void drawPixelXYF(float x, float y, CRGB col)
    {
        byte ax = byte(x);
        byte xsh = (x - ax) * 255;
        byte ay = byte(y);
        byte ysh = (y - ay) * 255;
        CRGB colP1 = colorsmear(col, CRGB(0, 0, 0), xsh);
        CRGB colP2 = colorsmear(CRGB(0, 0, 0), col, xsh);
        CRGB col1 = colorsmear(colP1, CRGB(0, 0, 0), ysh);
        CRGB col2 = colorsmear(colP2, CRGB(0, 0, 0), ysh);
        CRGB col3 = colorsmear(CRGB(0, 0, 0), colP1, ysh);
        CRGB col4 = colorsmear(CRGB(0, 0, 0), colP2, ysh);

        g()->leds[XY(ax, ay)] += col1;
        g()->leds[XY(ax + 1, ay)] += col2;
        g()->leds[XY(ax, ay + 1)] += col3;
        g()->leds[XY(ax + 1, ay + 1)] += col4;
    }

    void digMaze(int x, int y)
    {
        int x1, y1;
        int x2, y2;
        int dx, dy;
        int dir, count;

        dir = random(10) % 4;
        count = 0;
        while (count < 4)
        {
            dx = 0;
            dy = 0;
            switch (dir)
            {
            case 0:
                dx = 1;
                break;
            case 1:
                dy = 1;
                break;
            case 2:
                dx = -1;
                break;
            default:
                dy = -1;
                break;
            }
            x1 = x + dx;
            y1 = y + dy;
            x2 = x1 + dx;
            y2 = y1 + dy;
            if (x2 > 0 && x2 < M_WIDTH && y2 > 0 && y2 < M_HEIGHT && maze[x1][y1] && maze[x2][y2])
            {
                maze[x1][y1] = 0;
                maze[x2][y2] = 0;
                x = x2;
                y = y2;
                dir = random(10) % 4;
                count = 0;
            }
            else
            {
                dir = (dir + 1) % 4;
                count += 1;
            }
        }
    }

    void generateMaze()
    {
        for (byte y = 0; y < M_HEIGHT; y++)
        {
            for (byte x = 0; x < M_WIDTH; x++)
            {
                maze[x][y] = 1;
            }
        }
        maze[1][1] = 0;
        for (byte y = 1; y < M_HEIGHT; y += 2)
        {
            for (byte x = 1; x < M_WIDTH; x += 2)
            {
                digMaze(x, y);
            }
        }
        maze[0][1] = 0;
        maze[M_WIDTH - 2][M_HEIGHT - 1] = 0;
    }

  public:
    PatternSMMaze2() : LEDStripEffect(EFFECT_MATRIX_SMMAZE2, "Maze 2")
    {
    }

    PatternSMMaze2(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
    }

    void Draw() override
    {
        // Because we can restart, we don't move the start into Start() on this one.
        if (start)
        {
            start = 0;
            color = random();
            generateMaze();
            posX = 0, posY = 1;
            checkFlag = 1;
            tale = random() % 2;
            for (byte x = 0; x < LED_COLS; x++)
            {
                for (byte y = 0; y < LED_ROWS; y++)
                {
                    g()->leds[XY(x, y)] = (maze[x + M_SHIFT_X][y + M_SHIFT_Y]) ? CHSV(color, 200, 255) : CHSV(0, 0, 0);
                }
            }
        }
        if (!tale)
        {
            for (byte x = 0; x < LED_COLS; x++)
            {
                for (byte y = 0; y < LED_ROWS; y++)
                {
                    g()->leds[XY(x, y)] = (maze[x + M_SHIFT_X][y + M_SHIFT_Y]) ? CHSV(color, 200, 255) : CHSV(0, 0, 0);
                }
            }
        }
        if (checkFlag)
        {
            switch (Lookdir)
            {
            case 0:
                if (!maze[posX][posY - 1])
                {
                    Lookdir = 1;
                }
                break;
            case 1:
                if (!maze[posX - 1][posY])
                {
                    Lookdir = 2;
                }
                break;
            case 2:
                if (!maze[posX][posY + 1])
                {
                    Lookdir = 3;
                }
                break;
            case 3:
                if (!maze[posX + 1][posY])
                {
                    Lookdir = 0;
                }
                break;
            }
            while (true)
            {
                bool et1 = 0;
                switch (Lookdir)
                {
                case 0:
                    if (maze[posX + 1][posY])
                    {
                        Lookdir = 3;
                        et1 = 1;
                    }
                    break;
                case 1:
                    if (maze[posX][posY - 1])
                    {
                        Lookdir = 0;
                        et1 = 1;
                    }
                    break;
                case 2:
                    if (maze[posX - 1][posY])
                    {
                        Lookdir = 1;
                        et1 = 1;
                    }
                    break;
                case 3:
                    if (maze[posX][posY + 1])
                    {
                        Lookdir = 2;
                        et1 = 1;
                    }
                    break;
                }
                if (!et1)
                    break;
            }
            checkFlag = 0;
        }
        SubPos += 64;
        if (SubPos >= 255)
        {
            SubPos = 0;
            checkFlag = 1;
            switch (Lookdir)
            {
            case 0:
                posX += 1;
                break;
            case 1:
                posY -= 1;
                break;
            case 2:
                posX -= 1;
                break;
            case 3:
                posY += 1;
                break;
            }
        }
        switch (Lookdir)
        {
        case 0:
            drawPixelXYF(float(posX - M_SHIFT_X) + (float(SubPos) / 255.), (posY - M_SHIFT_Y), CHSV(0, 0, 255));
            break;
        case 1:
            drawPixelXYF((posX - M_SHIFT_X), float(posY - M_SHIFT_Y) - (float(SubPos) / 255.), CHSV(0, 0, 255));
            break;
        case 2:
            drawPixelXYF(float(posX - M_SHIFT_X) - (float(SubPos) / 255.), (posY - M_SHIFT_Y), CHSV(0, 0, 255));
            break;
        case 3:
            drawPixelXYF((posX - M_SHIFT_X), float(posY - M_SHIFT_Y) + (float(SubPos) / 255.), CHSV(0, 0, 255));
            break;
        }
        if ((posX == M_WIDTH - 2) & (posY == M_HEIGHT - 1))
            start = 1;
    }
};
