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

#include "defines.h"

#include "serials.h"
#include "iot_hub.h"
#include "ir_baby.h"

#include "aliyun_iot_sdk.h"

#include "aliot_client.h"

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

extern String g_aliot_region;


// private variable definitions
static bool force_disconnected = false;
static AliyunIoTSDK iot;


// public function definitions
int connectToAliyunIoT(PubSubClient &mqtt_client) {

    if (0 == iot.begin(mqtt_client, g_product_key.c_str(), g_device_name.c_str(), g_device_secret.c_str(), g_aliot_region.c_str())) {
        sendIrisKitConnect();
    }
    INFOLN("Aliyun IoT connected");
    return 0;
}

void aliotKeepAlive() {
    iot.loop();
}