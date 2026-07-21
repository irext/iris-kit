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

#include "defines.h"

#include "serials.h"
#include "iot_hub.h"
#include "iris_client.h"

#include "emq_client.h"


// external variable declarations
extern String g_mqtt_server;
extern String g_product_key;
extern String g_device_name;
extern String g_device_secret;

extern String g_mqtt_client_id;
extern String g_mqtt_user_name;
extern String g_mqtt_password;
extern String g_upstream_topic;
extern String g_downstream_topic;
extern int g_mqtt_port;


// private variable definitions
static bool force_disconnected = false;
static PubSubClient* emqx_client = nullptr;

// private function declarations


// public function definitions
int connectToEMQXBroker(PubSubClient &mqtt_client) {
    int retry_times = 0;

    if (nullptr == emqx_client) {
        emqx_client = &mqtt_client;
    }
    emqx_client->setServer(g_mqtt_server.c_str(), g_mqtt_port);

    force_disconnected = false;

    while (!force_disconnected && !emqx_client->connected() && retry_times < MQTT_RETRY_MAX) {
        INFOF("Connecting to MQTT Broker, client_id = %s, user_name = %s\n",
               g_mqtt_client_id.c_str(),
               g_mqtt_user_name.c_str());
        if (emqx_client->connect(g_mqtt_client_id.c_str(), g_mqtt_user_name.c_str(), g_mqtt_password.c_str())) {
            INFOF("Connected to MQTT broker\n");
        } else {
            ERRORF("Failed to connect to MQTT broker, rc = %d\n", emqx_client->state());
            INFOF("Try again in 5 seconds\n");
            retry_times++;
            delay(MQTT_RETRY_DELAY);
        }
    }
    if (emqx_client->connected()) {
        INFOF("IoT connect done\n");
    } else {
        ERRORF("IoT failed to connect\n");
        ESP.restart();
    }
    return 0;
}

void emqxClientKeepAlive(PubSubClient& mqtt_client) {
    (void) mqtt_client;
    emqx_client->loop();
}

int disconnectFromEMQXBroker() {
    force_disconnected = true;
    emqx_client->disconnect();
    return 0;
}