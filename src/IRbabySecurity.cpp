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

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <AliyunIoTSDK.h>

#include "IRbabySerial.h"
#include "IRbabyAlink.h"
#include "das.h"

// global SOC session abstractive object
static void* dasSession = NULL;

int lsocClientInit(const char* productName, const char* deviceName) {
    INFOLN("initialize SOC client");
    dasSession = das_init(productName, deviceName);

    if (NULL == dasSession) {
        INFOLN("failed to initialize SOC client !!");
        return -1;
    }

    INFOLN("SOC client initialized");
    AliyunIoTSDK session = getSession();
    das_connection(dasSession, securityPublish, &session);

    return 0;
}

void setDas2Alink() {
    // NOTE: if the Communication-processor has MQTT support,
    // set connection to virutal socket pointer
    // otherwise, set to espatblished Alink publish prototype
    
}

void lsocInfoReport() {
    
}