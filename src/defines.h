/**
 *
 * Copyright (c) 2020-2022 IRbaby-IRext
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _DEFINES_H
#define _DEFINES_H

typedef unsigned int uint32_t;
typedef int int32_t;
typedef unsigned short uint16_t;
typedef short int16_t;
typedef unsigned char uint8_t;

#define FIRMWARE_VERSION "0.2.7" // version name
#define VERSION_CODE 1          // version code

/* log settings */
#define BAUD_RATE 115200
#ifndef LOG_DEBUG
    #define LOG_DEBUG false
#endif
#ifndef LOG_INFO
    #define LOG_INFO true
#endif
#ifndef LOG_ERROR
    #define LOG_ERROR true
#endif

/* ----------------- user settings ----------------- */
/* mqtt settings */
#define MQTT_CHECK_INTERVALS 30      // seconds
#define MQTT_CONNECT_WAIT_TIME 20000 // MQTT 连接等待时间

/* receive disable */
#define DISABLE_SIGNAL_INTERVALS 600 // seconds

#define SAVE_DATA_INTERVALS 300 // seconds
// uncomment below to enable RF
// #define USE_RF

// uncomment below to enable upload chip id to remote server
// #define USE_INFO_UPLOAD

/* ----------------- default pin setting --------------- */
/* reset pin */
#define RESET_PIN 2

/* 315 RF pin */
#define T_315 5
#define R_315 4

/* 433 RF pin */
#define T_433 14
#define R_433 12

/* IR pin */
#define T_IR 14
#define R_IR 12

/* ----------------- lsoc setting --------------- */
/* lsoc heart beat cycle */
#define LSOC_HB_CYCLE  1800

#endif  // _DEFINES_H
