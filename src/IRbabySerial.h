/**
 *
 * Copyright (c) 2020-2021 IRbaby-IRext
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

#ifndef IRBABY_SERIAL_H
#define IRBABY_SERIAL_H

#include <Arduino.h>
#include "defines.h"

// generic COM debug

#define DEBUGLN(...) \
    {if (LOG_DEBUG) { Serial.printf("DEBUG:\t"); Serial.println(__VA_ARGS__);}} 
#define DEBUGF(...) \
    {if (LOG_DEBUG) { Serial.printf("DEBUG:\t"); Serial.printf(__VA_ARGS__);}} 
#define DEBUG(...)  \
    {if (LOG_DEBUG) { Serial.printf("DEBUG:\t"); Serial.print(__VA_ARGS__);}} 

#define INFOLN(...) \
    {if (LOG_INFO) { Serial.printf("INFO:\t"); Serial.println(__VA_ARGS__);}} 
#define INFOF(...) \
    {if (LOG_INFO) { Serial.printf("INFO:\t"); Serial.printf(__VA_ARGS__);}} 
#define INFO(...)  \
    {if (LOG_INFO) { Serial.printf("INFO:\t"); Serial.print(__VA_ARGS__);}} 

#define ERRORLN(...) \
    {if (LOG_ERROR) { Serial.printf("ERROR:\t"); Serial.println(__VA_ARGS__);}} 
#define ERRORF(...) \
    {if (LOG_ERROR) { Serial.printf("ERROR:\t"); Serial.printf(__VA_ARGS__);}} 
#define ERROR(...)  \
    {if (LOG_ERROR) { Serial.printf("ERROR:\t"); Serial.print(__VA_ARGS__);}}
    
#endif // IREASY_SERIAL_H