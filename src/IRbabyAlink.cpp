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

#include <Arduino.h>

#include "IRbabySerial.h"
#include "IRbabyAlink.h"
#include "IRbabyGlobal.h"

#define TOPIC_NAME_MAX  (64)

#define PRODUCT_KEY     "a1WlzsJh50b"
#define DEVICE_NAME     "IRIS_Kit_Dev"
#define DEVICE_SECRET   "9df2c6b48e4c66519718cc236fc9fb79"
#define REGION_ID       "cn-shanghai"

#define USER_NAME       "strawmanbobi@irext.net"

static AliyunIoTSDK iot;
static char IRIS_UPSTREAM_TOPIC[TOPIC_NAME_MAX] = { 0 };
static ep_state_t endpoint_state = FSM_IDLE;

static void registerCallback();
static void irisAlinkCallback(const char *topic, uint8_t *data, int length);
static void sendIrisKitHeartBeat();

void connectToAliyunIoT() {
    INFOLN("Try connecting to Aliyun IoT");
    iot.begin(wifi_client, PRODUCT_KEY, DEVICE_NAME, DEVICE_SECRET, REGION_ID);
    INFOLN("Aliyun IoT connect done");
    snprintf(IRIS_UPSTREAM_TOPIC, TOPIC_NAME_MAX - 1, "/%s/%s/user/iris/upstream", PRODUCT_KEY,
             DEVICE_NAME);
    registerCallback();
}

void checkAlinkMQTT() {
    iot.loop();
    sendIrisKitHeartBeat();
}

// not only for IRIS related topic based session
void sendRawData(const char* topic, const uint8_t *data, int length) {
    iot.sendCustomData(topic, data, length);
}

AliyunIoTSDK getSession() {
    return iot;
}

static void registerCallback() {
    iot.registerCustomCallback(irisAlinkCallback);
}

static void irisAlinkCallback(const char *topic, uint8_t *data, int length) {
    INFO("IRIS Alink callback triggerd : topic = ");
    INFO(topic);
    INFO(", data = ");
    INFO((char*)data);
    INFO(", length = ");
    INFOLN(length);
}

static void sendIrisKitHeartBeat() {

}