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

#ifndef IRIS_KIT_GLOBAL_H
#define IRIS_KIT_GLOBAL_H

#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <WiFiClient.h>

/* goable json variable */
extern StaticJsonDocument<1024> recv_msg_doc;
extern StaticJsonDocument<1024> send_msg_doc;

extern WiFiManager wifi_manager;
extern WiFiClient wifi_client;

extern uint8_t ir_send_pin;
extern uint8_t ir_receive_pin;
#ifdef USE_RF
    extern uint8_t rf315_send_pin;
    extern uint8_t rf315_receive_pin;
    extern uint8_t rf433_send_pin;
    extern uint8_t rf433_receive_pin;
#endif

#endif // IRIS_KIT_GLOBAL_H