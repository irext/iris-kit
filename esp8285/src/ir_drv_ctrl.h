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

#ifndef IR_DRV_CTRL_H
#define IR_DRV_CTRL_H

#include <Arduino.h>
#include <IRsend.h>
#include <IRrecv.h>

#include "ir_decode.h"

void loadIRPin(uint8_t send_pin, uint8_t recv_pin);

void enableIRIn();

void disableIRIn();

bool downloadBin(int remote_id);

bool sendIR(String file_name);

bool emitIR(String timing);

bool sendCommand(String file_name, int key);

void sendStatus(String file_name, t_remote_ac_status status);

void prepareStudyIR();

void cancelStudyIR();

int completeStudyIR(String &ir_data);

void recvIR();

bool saveReceived(String &results);

bool removeReceived();

int loadReceived(String &ir_data);

void initAC(String);

#endif // IR_DRV_CTRL_H