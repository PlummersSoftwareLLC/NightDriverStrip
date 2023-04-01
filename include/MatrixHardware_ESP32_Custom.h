/*
 * SmartMatrix Library - Hardware-Specific Header File (ESP32 pinout collection)
 *
 * Copyright (c) 2020 Louis Beaudoin (Pixelmatix)
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

 // Note: only one MatrixHardware_*.h file should be included per project

#ifndef MATRIX_HARDWARE_H
#define MATRIX_HARDWARE_H
// formula used is 80000000L/(cfg->clkspeed_hz + 1), must result in >=2.  Acceptable values 26.67MHz, 20MHz, 16MHz, 13.34MHz...
#define ESP32_I2S_CLOCK_SPEED (20000000UL)

//#define GPIOPINOUT ESP32_FORUM_PINOUT_WITH_LATCH // note this mode is untested as of 2018-05-17 - not being used anymore now that SmartMatrix Shield is available
//#define GPIOPINOUT SMARTLED_SHIELD_V0_PINOUT

//Upper half RGB
#define BIT_R1  (1<<0)   
#define BIT_G1  (1<<1)   
#define BIT_B1  (1<<2)   
//Lower half RGB
#define BIT_R2  (1<<3)   
#define BIT_G2  (1<<4)   
#define BIT_B2  (1<<5)   

// Control Signals
#define BIT_LAT (1<<6) 
#define BIT_OE  (1<<7)  

#define BIT_A (1<<8)    
#define BIT_B (1<<9)    
#define BIT_C (1<<10)   
#define BIT_D (1<<11)   
#define BIT_E (1<<12)   



    // #pragma message "MatrixHardware: Custom Wrover Kit wiring"

    // ADDX is output directly using GPIO
    #define CLKS_DURING_LATCH   0 
    #define MATRIX_I2S_MODE I2S_PARALLEL_BITS_16
    #define MATRIX_DATA_STORAGE_TYPE uint16_t

    /*
    HUB 75
    01 02 B0
    03 04 Gnd
    05 06 G1
    07 08 E

    09 10 B
    11 12 D
    13 14 STB/Latch
    15 16 Gnd
                        ESP32 pin / comment
    1   R0  2   Red Data (columns 1-16)
    2   G0  15  Green Data (columns 1-16)

    3   B0  4   Blue Data (columns 1-16)
    4   GND GND Ground
    
    5   R1  16/RX2  Red Data (columns 17-32)
    6   G1  27  Green Data (columns 17-32)
    
    7   B1  17/TX2  Blue Data (columns 17-32)
    8   E   12  Demux Input E for 64x64 panels
   
    9   A   5   Demux Input A0
    10  B   18  Demux Input A1

    11  C   19  Demux Input A2
    12  D   21  Demux Input E1, E3 (32x32 panels only)
    
    13  CLK 22  LED Drivers' Clock
    14  STB 26  LED Drivers' Latch
    
    15  OE  25  LED Drivers' Output Enable
    16  GND GND Ground
    */ 

    /* Mesmerizer Classic Breadboard Setup */
    /*
    #define R1_PIN  GPIO_NUM_12
    #define G1_PIN  GPIO_NUM_13
    #define B1_PIN  GPIO_NUM_26
    #define R2_PIN  GPIO_NUM_22
    #define G2_PIN  GPIO_NUM_27
    #define B2_PIN  GPIO_NUM_19
    #define A_PIN   GPIO_NUM_5
    #define B_PIN   GPIO_NUM_23
    #define C_PIN   GPIO_NUM_0
    #define D_PIN   GPIO_NUM_21
    #define E_PIN   GPIO_NUM_25
    #define LAT_PIN GPIO_NUM_18
    #define OE_PIN  GPIO_NUM_4
    #define CLK_PIN GPIO_NUM_2
    */

    #define R1_PIN  GPIO_NUM_2                  
    #define G1_PIN  GPIO_NUM_0                  
    #define B1_PIN  GPIO_NUM_32                 
    #define R2_PIN  GPIO_NUM_25                 
    #define G2_PIN  GPIO_NUM_33                 
    #define B2_PIN  GPIO_NUM_27                 
    #define A_PIN   GPIO_NUM_5                  
    #define B_PIN   GPIO_NUM_4                  
    #define C_PIN   GPIO_NUM_19                 
    #define D_PIN   GPIO_NUM_18                 
    #define E_PIN   GPIO_NUM_26                 
    #define LAT_PIN GPIO_NUM_21                 
    #define OE_PIN  GPIO_NUM_23                 
    #define CLK_PIN GPIO_NUM_22                 

//#define DEBUG_PINS_ENABLED
//#define DEBUG_1_GPIO    GPIO_NUM_13
//#define DEBUG_2_GPIO    GPIO_NUM_12

#else
    #pragma GCC error "Multiple MatrixHardware*.h files included"
#endif
