/**
 *
 * Copyright (c) 2020-2024 IRbaby-IRext
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

/* ----------------- version defs ----------------- */
#define FIRMWARE_VERSION              "1.3.0"
#define VERSION_CODE                  (2)

/* ----------------- log settings ----------------- */
#define BAUD_RATE                     (115200)
#ifndef LOG_DEBUG
    #define LOG_DEBUG false
#endif
#ifndef LOG_INFO
    #define LOG_INFO true
#endif
#ifndef LOG_ERROR
    #define LOG_ERROR true
#endif

/* ----------------- auth settings ----------------- */
#define CREDENTIAL_INIT_RETRY_MAX     (3)
#define SYSTEM_DELAY                  (2000)

/* ----------------- iot settings -----------------*/
#define MQTT_HOST_REL                 "iot.irext.net"
#define MQTT_CHECK_INTERVALS          (120)
#define MQTT_CONNECT_WAIT_TIME        (20000)
#define MQTT_RETRY_DELAY              (5000)
#define MQTT_RETRY_MAX                (5)


/* ----------------- storage settings ----------------- */
#define DISABLE_SIGNAL_INTERVALS      (600)
#define SAVE_DATA_INTERVALS           (300)

/* ----------------- pin settings --------------- */
#define RESET_PIN                     (2)
#define T_IR                          (4)
#define R_IR                          (14)

#endif  // _DEFINES_H
