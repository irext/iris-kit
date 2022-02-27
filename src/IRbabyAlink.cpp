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
#include <WString.h>

#include "IRbabySerial.h"
#include "IRbabyAlink.h"
#include "IRbabyGlobal.h"

#include "IRbaby.h"

#define TOPIC_NAME_MAX    (64)
#define IOT_RETRY_MAX     (3)

String g_product_key = "";
String g_device_name = "";
String g_device_secret = "";
String g_region_id = "cn-shanghai";

static AliyunIoTSDK iot;
static char IRIS_UPSTREAM_TOPIC[TOPIC_NAME_MAX] = { 0 };
static ep_state_t endpoint_state = FSM_IDLE;

static void registerCallback();
static void irisAlinkCallback(const char *topic, uint8_t *data, int length);

static int iot_retry = 0;

static void sendIrisKitHeartBeat();

void connectToAliyunIoT() {
    INFOF("Try connecting to Aliyun IoT : %s, %s, %s, %s\n",
          g_product_key.c_str(), g_device_name.c_str(), g_device_secret.c_str(), g_region_id.c_str());
    iot.begin(wifi_client, g_product_key.c_str(), g_device_name.c_str(), g_device_secret.c_str(),
              g_region_id.c_str());
    INFOLN("Aliyun IoT connect done");
    snprintf(IRIS_UPSTREAM_TOPIC, TOPIC_NAME_MAX - 1, "/%s/%s/user/iris/upstream",
             g_product_key.c_str(), g_device_name.c_str());
    registerCallback();
}

void checkAlinkMQTT() {
    int mqttStatus = 0;
    mqttStatus = iot.loop();

    if (0 == mqttStatus) {
        iot_retry = 0;
        sendIrisKitHeartBeat();
    } else {
        INFOF("Alink MQTT check failed, retry = %d\n", iot_retry);
        iot_retry++;
    }
    if (iot_retry >= IOT_RETRY_MAX) {
        ERRORLN("Alink could not established, something went wrong, reset...");
        factoryReset();
    }
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