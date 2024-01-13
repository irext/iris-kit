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
#include <PubSubClient.h>

#include "IRISKitSerials.h"
#include "IRISKitIoT.h"
#include "IRISKitIRbaby.h"
#include "IRISKitGlobal.h"

#include "IRISKit.h"

#define IRIS_KIT_PK_DEV        "a1WlzsJh50b"
#define IRIS_KIT_PK_REL        "a1ihYt1lqGH"
#define TOPIC_DOWNSTREAM_DEV   "/user/iris_kit_downstream_dev"
#define TOPIC_UPSTREAM_DEV     "/user/iris_kit_upstream_dev"
#define TOPIC_DOWNSTREAM_REL   "/user/iris_kit_downstream"
#define TOPIC_UPSTREAM_REL     "/user/iris_kit_upstream"

String g_mqtt_server = "";
String g_product_key = "";
String g_device_name = "";
String g_device_secret = "";

String g_mqtt_client_id = "";
String g_mqtt_user_name = "";
String g_mqtt_password = "";
String g_upstream_topic = "";
String g_downstream_topic = "";
int g_mqtt_port = 1883;

int g_app_id = 0;

static bool downstream_topic_subscribed = false;
static ep_state_t endpoint_state = FSM_IDLE;

static int connectToMQTTBroker();

static void irisIrextIoTCallback(char *topic, uint8_t *data, uint32_t length);

static int iot_retry = 0;

static PubSubClient mqtt_client(wifi_client);

int connectToIrextIoT() {
    downstream_topic_subscribed = false;
    int conn_ret = -1;

    if (g_product_key.equals(IRIS_KIT_PK_DEV)) {
        g_upstream_topic = "/" + g_product_key + "/" + g_device_name + TOPIC_UPSTREAM_DEV;
        g_downstream_topic = "/" + g_product_key + "/" + g_device_name + TOPIC_DOWNSTREAM_DEV;
    } else if (g_product_key.equals(IRIS_KIT_PK_REL)) {
        g_upstream_topic = "/" + g_product_key + "/" + g_device_name + TOPIC_UPSTREAM_REL;
        g_downstream_topic = "/" + g_product_key + "/" + g_device_name + TOPIC_DOWNSTREAM_REL;
    } else {
        ERRORF("IRIS Kit release key is not supported yet\n");
        factoryReset();
        return -1;
    }

    g_mqtt_user_name = getDeviceID();

    INFOF("Try connecting to IRext IoT %s:%d, client_id = %s, user_name = %s, password.size = %d\n",
          g_mqtt_server.c_str(), g_mqtt_port,
          g_mqtt_client_id.c_str(), g_mqtt_user_name.c_str(), g_mqtt_password.length());

    mqtt_client.setBufferSize(2048);
    mqtt_client.setServer(g_mqtt_server.c_str(), g_mqtt_port);
    mqtt_client.setCallback(irisIrextIoTCallback);
    conn_ret = connectToMQTTBroker();

    if (0 != conn_ret) {
        ERRORLN("Something may went wrong with your credential, please retry connect to Wifi...");
        factoryReset();
        return -1;
    }

    // send connect request
    sendIrisKitConnect();

    return 0;
}

void irextIoTKeepAlive() {
    if (!mqtt_client.connected()) {
        connectToMQTTBroker();
    }
    mqtt_client.loop();
}

// not only for IRIS related topic based session
void sendData(const char* topic, const uint8_t *data, int length) {
    mqtt_client.publish(topic, data, length);
}

void* getSession() {
    return &mqtt_client;
}

void checkIrextIoT() {
    if (mqtt_client.connected()) {
        sendIrisKitHeartBeat();
    }
}

static int connectToMQTTBroker() {
    int retry_times = 0;

    while (!mqtt_client.connected() && retry_times < MQTT_RETRY_MAX) {
        INFOF("Connecting to MQTT Broker as %s.....\n", g_mqtt_client_id.c_str());
        if (mqtt_client.connect(g_mqtt_client_id.c_str(), g_mqtt_user_name.c_str(), g_mqtt_password.c_str())) {
            INFOF("Connected to MQTT broker\n");
            mqtt_client.subscribe(g_downstream_topic.c_str());
        } else {
            ERRORF("Failed to connect to MQTT broker, rc = %d\n", mqtt_client.state());
            INFOF(" try again in 5 seconds\n");
            retry_times++;
            delay(MQTT_RETRY_DELAY);
        }
    }
    if (mqtt_client.connected()) {
        INFOF("IRext IoT connect done\n");
        return 0;
    } else {
        ERRORF("IRext IoT failed to connect\n");
        return -1;
    }
}

static void irisIrextIoTCallback(char *topic, uint8_t *data, uint32_t length) {
    INFOF("downstream message received, topic = %s, length = %d\n", topic, length);
    if (NULL != g_downstream_topic.c_str() && 0 == strcmp(topic, g_downstream_topic.c_str())) {
        handleIrisKitMessage((const char*) data, length);
    }
}
