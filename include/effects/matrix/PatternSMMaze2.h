#pragma once

#include "effectmanager.h"

// Derived from https://editor.soulmatelights.com/gallery/1956-maze
// We have another maze generator, but this is self-solving.

class PatternSMMaze2 : public LEDStripEffect
{
  private:
    static constexpr uint M_HEIGHT = MATRIX_HEIGHT + !(MATRIX_HEIGHT % 2);
    static constexpr uint M_WIDTH = MATRIX_WIDTH + !(MATRIX_WIDTH % 2);
    static constexpr uint M_SHIFT_X = !(MATRIX_WIDTH % 2);
    static constexpr uint M_SHIFT_Y = !(MATRIX_HEIGHT % 2);
    bool maze[M_WIDTH][M_HEIGHT];
    bool start = true;
    unsigned int posX{1}, posY{1};
    byte Lookdir{0};
    bool checkFlag{true};
    bool tale = {true};
    int SubPos{0};
    uint8_t speed = 255; // We move this fraction of 255 ths pixel per Draw().

    void drawPixelXY(int8_t x, int8_t y, CRGB color) const
    {
        y = MATRIX_HEIGHT - 1 - y; // Invert y for NightDriver
        if (!g()->isValidPixel(x, y))
            return;
        g()->leds[XY(x, y)] = color;
    }

    void drawPixelXYF(float x, float y, CRGB col) const
    {
        drawPixelXY(x, y, col);
    }

    void digMaze(int x, int y)
    {
        int x1, y1;
        int x2, y2;
        int dx, dy;
        int dir, count;

        dir = random8() % 4;
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
                dir = random8(10) % 4;
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
        randomSeed(millis());

        for (unsigned int x = 0; x < M_WIDTH; x++)
        {
            for (unsigned int y = 0; y < M_HEIGHT; y++)
            {
                maze[x][y] = 1;
            }
        }
        maze[1][1] = 0;
        for (unsigned int y = 1; y < M_HEIGHT; y += 2)
        {
            for (unsigned int x = 1; x < M_WIDTH; x += 2)
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
        uint8_t color;
        if (start)
        {
            start = 0;
            color = random8();
            generateMaze();
            posX = 0, posY = 1;
            checkFlag = 1;
            tale = random8() % 2;
            for (unsigned int x = 0; x < MATRIX_WIDTH; x++)
            {
                for (unsigned int y = 0; y < MATRIX_HEIGHT; y++)
                {
                    drawPixelXY(x, y, maze[x + M_SHIFT_X][y + M_SHIFT_Y] ? CHSV(color, 200, 255) : CHSV(0, 0, 0));
                }
            }
        }
        if (!tale)
        {
            for (unsigned int x = 0; x < MATRIX_WIDTH; x++)
            {
                for (unsigned int y = 0; y < MATRIX_HEIGHT; y++)
                {
                    drawPixelXY(x, y, maze[x + M_SHIFT_X][y + M_SHIFT_Y] ? CHSV(color, 200, 255) : CHSV(0, 0, 0));
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
        SubPos += speed;
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
        if ((posX == M_WIDTH - 2) & (posY == M_HEIGHT - 1)) // Exit position
            start = 1;
    }

    // Admit surrender keeping the VU meter out of the maze. Banish it.
    bool CanDisplayVUMeter() const override
    {
        return false;
    }
};
