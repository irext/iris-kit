/**
*
 * Copyright (c) 2020-2026 IRext Opensource Organization
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

#include <Arduino.h>
#include "led_drv.h"
#include "defines.h"


// public function definitions
void initLEDs() {
    if (-1 == LED_SEND_PIN || -1 == LED_RECV_PIN) {
        return;
    }
    pinMode(LED_SEND_PIN, OUTPUT);
    pinMode(LED_RECV_PIN, OUTPUT);
    digitalWrite(LED_SEND_PIN, HIGH);
    digitalWrite(LED_RECV_PIN, HIGH);
}

void setSendLED(bool on) {
    if (-1 == LED_SEND_PIN) {
        return;
    }
    digitalWrite(LED_SEND_PIN, on ? LOW : HIGH);
}

void setRecvLED(bool on) {
    if (-1 == LED_RECV_PIN) {
        return;
    }
    digitalWrite(LED_RECV_PIN, on ? LOW : HIGH);
}

void blinkSendLED(int times, int delay_ms) {
    if (-1 == LED_SEND_PIN) {
        return;
    }
    for (int i = 0; i < times; i++) {
        digitalWrite(LED_SEND_PIN, LOW);
        delay(delay_ms);
        digitalWrite(LED_SEND_PIN, HIGH);
        delay(delay_ms);
    }
}

void blinkRecvLED(int times, int delay_ms) {
    if (-1 == LED_RECV_PIN) {
        return;
    }
    for (int i = 0; i < times; i++) {
        digitalWrite(LED_RECV_PIN, LOW);
        delay(delay_ms);
        digitalWrite(LED_RECV_PIN, HIGH);
        delay(delay_ms);
    }
}
