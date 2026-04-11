/**
 *
 * Copyright (c) 2020-2025 IRext Opensource Organization
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

#include "serials.h"
#include "iot_hub.h"
#include "iris_client.h"
#include "global.h"

#include "emq_client.h"
#include "aliot_client.h"
#include "iris_kit.h"

#include "iot_hub.h"


// external variable declarations
extern int g_runtime_env;


// public variable definitions
String g_mqtt_server = "";
String g_product_key = "";
String g_device_name = "";
String g_device_secret = "";
String g_device_token = "";

String g_mqtt_client_id = "";
String g_mqtt_user_name = "";
String g_mqtt_password = "";
String g_upstream_topic = "";
String g_downstream_topic = "";

String g_aliot_region = "cn-shanghai";
int g_mqtt_port = 1883;

int g_app_id = 0;
mqtt_type_t g_mqtt_type = MQTT_TYPE_MAX;
boolean g_subscribed = false;
unsigned long last_check_time = 0UL;

// private variable definitions
static bool downstream_topic_subscribed = false;


// private function declarations
static void irisIoTCallback(char *topic, uint8_t *data, uint32_t length);

static int iot_retry = 0;

static PubSubClient g_mqtt_client(wifi_client);


// public function definitions
int connectIot() {
    downstream_topic_subscribed = false;
    int conn_ret = -1;

    if (g_runtime_env == RUNTIME_DEBUG) {
        g_upstream_topic = "/" + g_product_key + "/" + g_device_name + TOPIC_UPSTREAM_DEV;
        g_downstream_topic = "/" + g_product_key + "/" + g_device_name + TOPIC_DOWNSTREAM_DEV;
    } else if (g_runtime_env == RUNTIME_RELEASE) {
        g_upstream_topic = "/" + g_product_key + "/" + g_device_name + TOPIC_UPSTREAM_REL;
        g_downstream_topic = "/" + g_product_key + "/" + g_device_name + TOPIC_DOWNSTREAM_REL;
    } else {
        ERRORF("Release key is not supported yet\n");
        factoryReset();
        return -1;
    }

    // g_mqtt_user_name act as clientId for aliot and as userName for EMQX
    g_mqtt_user_name = getDeviceID();

    INFOF("Try connecting to AliyunIoT, product_key = %s, device_name = %s, device_secret = %s\n",
            g_product_key.c_str(), g_device_name.c_str(), g_device_token.c_str());
    conn_ret = connectToAliot(g_mqtt_client);

    if (0 != conn_ret) {
        INFOF("Try connecting to EMQX %s:%d, client_id = %s, user_name = %s, password.size = %d\n",
            g_mqtt_server.c_str(), g_mqtt_port,
            g_mqtt_client_id.c_str(), g_mqtt_user_name.c_str(), g_mqtt_password.length());
        conn_ret = connectToEMQXBroker(g_mqtt_client);
        if (0 == conn_ret) {
            g_mqtt_type = MQTT_TYPE_EMQX;
        }
    } else {
        g_mqtt_type = MQTT_TYPE_ALIOT;
    }

    if (0 != conn_ret) {
        ERRORF("Something may went wrong with your credential, please retry connect to Wifi...\n");
        factoryReset();
        return -1;
    }

    if (!g_subscribed) {
        g_mqtt_client.setCallback(iotCallback);
        g_mqtt_client.subscribe(g_downstream_topic.c_str());
        g_subscribed = true;
    }

    // send connect request
    sendIrisKitConnect();

    return conn_ret;
}

void keepAliveIot() {
    if (g_mqtt_client.connected()) {
        if (MQTT_TYPE_ALIOT == g_mqtt_type) {
            aliotKeepAlive(g_mqtt_client);
        } else if (MQTT_TYPE_EMQX == g_mqtt_type) {
            emqxClientKeepAlive(g_mqtt_client);
        }
    }
    unsigned long current_time = millis();
    if (current_time - last_check_time > 10000) {
        if (!g_mqtt_client.connected()) {
            ERRORF("MQTT client is not connected, force disconnect and retry\n");
            g_mqtt_client.disconnect();
            g_mqtt_client.unsubscribe(g_downstream_topic.c_str());
            g_subscribed = false;
            connectIot();
            last_check_time = current_time;
        }
    }
}

// not only for IRIS related topic based session
void sendData(const char* topic, const uint8_t *data, int length) {
    g_mqtt_client.publish(topic, data, length);
}

void* getSession() {
    return &g_mqtt_client;
}

void checkIot() {
    if (g_mqtt_client.connected()) {
        INFOF("Send iris kit heart beat\n");
        sendIrisKitHeartBeat();
    }
}

void iotCallback(char *topic, uint8_t *data, uint32_t length) {
    INFOF("Downstream message received, topic = %s, length = %d\n", topic, length);
    if (nullptr != g_downstream_topic.c_str() && 0 == strcmp(topic, g_downstream_topic.c_str())) {
        handleIrisKitMessage((const char*) data, length);
    }
}