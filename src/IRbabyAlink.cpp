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
#include "IRbabyIRIS.h"
#include "IRbabyGlobal.h"

#include "IRbaby.h"

#define TOPIC_NAME_MAX         (64)
#define IOT_RETRY_MAX          (3)

#define IRIS_KIT_PK_DEV        "a1WlzsJh50b"
#define IRIS_KIT_PK_REL        "a1ihYt1lqGH"
#define TOPIC_DOWNSTREAM_DEV   "/user/iris/downstream_dev"
#define TOPIC_UPSTREAM_DEV     "/user/iris/upstream_dev"
#define TOPIC_DOWNSTREAM_REL   "/user/iris/downstream"
#define TOPIC_UPSTREAM_REL     "/user/iris/upstream"

String g_product_key = "";
String g_device_name = "";
String g_device_secret = "";
String g_region_id = "cn-shanghai";
String g_upstream_topic = "";
String g_downstream_topic = "";
int g_app_id = 0;


static bool downstream_topic_subscribed = false;
static AliyunIoTSDK iot;
static ep_state_t endpoint_state = FSM_IDLE;

static void registerCallback(const char* topic, int qos);
static void irisAlinkCallback(const char *topic, uint8_t *data, int length);

static int iot_retry = 0;

void connectToAliyunIoT() {
    downstream_topic_subscribed = false;

    if (g_product_key.equals(IRIS_KIT_PK_DEV)) {
        g_upstream_topic = "/" + g_product_key + "/" + g_device_name + TOPIC_UPSTREAM_DEV;
        g_downstream_topic = "/" + g_product_key + "/" + g_device_name + TOPIC_DOWNSTREAM_DEV;
    } else if (g_product_key.equals(IRIS_KIT_PK_REL)) {
        g_upstream_topic = "/" + g_product_key + "/" + g_device_name + TOPIC_UPSTREAM_REL;
        g_downstream_topic = "/" + g_product_key + "/" + g_device_name + TOPIC_DOWNSTREAM_REL;
    } else {
        ERRORF("IRIS Kit release key is not supported yet\n");
        return;
    }

    INFOF("Try connecting to Aliyun IoT : %s, %s, %s, %s\n",
          g_product_key.c_str(), g_device_name.c_str(), g_device_secret.c_str(), g_region_id.c_str());

    if (0 == iot.begin(wifi_client, g_product_key.c_str(), g_device_name.c_str(), g_device_secret.c_str(),
              g_region_id.c_str())) {
        sendIrisKitConnect();
    }
    INFOLN("Aliyun IoT connect done");
    
}

void aliotKeepAlive() {
    iot.loop();
}

// not only for IRIS related topic based session
void sendRawData(const char* topic, const uint8_t *data, int length) {
    iot.sendCustomData(topic, data, length);
}

AliyunIoTSDK getSession() {
    return iot;
}

static void registerCallback(const char* topic, int qos) {
    if (iot.subscribe(topic, qos)) {
        INFOLN("topic subscribed");
        iot.registerCustomCallback(irisAlinkCallback);
    }
}

static void irisAlinkCallback(const char *topic, uint8_t *data, int length) {
    INFOF("downstream message received, topic = %s, length = %d\n", topic, length);
    if (NULL != g_downstream_topic.c_str() && 0 == strcmp(topic, g_downstream_topic.c_str())) {
        handleIrisKitMessage((const char*) data, length);
    }
}

void checkAlinkMQTT() {
    int mqttStatus = 0;

    if (0 == mqttStatus) {
        iot_retry = 0;
        if (false == downstream_topic_subscribed) {
            INFOF("subscribe topic : %s\n", g_downstream_topic.c_str());
            registerCallback(g_downstream_topic.c_str(), 0);
            downstream_topic_subscribed = true;
        } else {
            sendIrisKitHeartBeat();
        }
    } else {
        INFOF("Alink MQTT check failed, retry = %d\n", iot_retry);
        iot_retry++;
    }
    if (iot_retry >= IOT_RETRY_MAX) {
        ERRORLN("Alink could not established, something went wrong, reset...");
        factoryReset();
    }
}