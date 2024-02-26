/**
 *
 * Copyright (c) 2020-2024 IRbaby-IRext
 *
 * Author: Strawmanbobi and Caffreyfans
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
#include <LittleFS.h>
#include <Ticker.h>
#include <WiFiUdp.h>

#include "defines.h"
#include "ir_emit.h"
#include "iot_hub.h"
#include "http_client.h"
#include "global.h"
#include "iris_client.h"
#include "serials.h"
#include "user_settings.h"
#include "utils.h"

#include "iris_kit.h"

// external variable declarations
extern char iris_server_address[];
extern char iris_credential_token[];
extern char iris_password[];

extern String g_mqtt_server;
extern String g_product_key;
extern String g_device_name;
extern String g_device_secret;
extern String g_device_token;
extern String g_mqtt_client_id;
extern String g_mqtt_password;
extern int g_app_id;

// public variable definitions
const unsigned long utcOffsetInMilliSeconds = 3600 * 1000;

int credential_init_retry = 0;
int g_runtime_env = RUNTIME_RELEASE;
iris_kit_settings_t iriskit_settings;
bool iris_kit_settings_loaded = false;


// private variable definitions
static Ticker iotCheckTask;            // IRext IoT MQTT check timer
static Ticker disableIRTask;           // disable IR receive
static Ticker disableRFTask;           // disable RF receive
static Ticker saveDataTask;            // save data


// private function declarations
static void wifiReset();

// public function definitions
void setup() {
    if (LOG_DEBUG || LOG_ERROR || LOG_INFO) {
        Serial.begin(BAUD_RATE);
    }

    pinMode(RESET_PIN, INPUT_PULLUP);
    pinMode(0, OUTPUT);
    digitalWrite(0, LOW);
    attachInterrupt(digitalPinToInterrupt(RESET_PIN), factoryReset, ONLOW);
    delay(SYSTEM_DELAY);

    Serial.clearWriteError();
    INFOLN();
    INFOLN("██╗██████╗ ██╗███████╗");
    INFOLN("██║██╔══██╗██║██╔════╝");
    INFOLN("██║██████╔╝██║███████╗");
    INFOLN("██║██╔══██╗██║╚════██║");
    INFOLN("██║██║  ██║██║███████║");
    INFOLN("╚═╝╚═╝  ╚═╝╚═╝╚══════╝");
    INFOLN("== IRIS Kit [1.3.0_r1] Powered by AliyunIoT ==");

    // try loading saved iriskit settings
    iriskit_settings.credential_token.clear();
    iriskit_settings.server_address.clear();
    iriskit_settings.password.clear();

    if (loadSettings()) {
        iriskit_settings = getIrisKitSettings();
        INFOLN("saved credentials loaded");
        INFOF("server address is empty ? %s\n", iriskit_settings.server_address.isEmpty() ? "yes" : "no");
        INFOF("credential is empty ? %s\n", iriskit_settings.credential_token.isEmpty() ? "yes" : "no");
        INFOF("password is empty ? %s\n", iriskit_settings.password.isEmpty() ? "yes" : "no");
    }
    if (!iriskit_settings.credential_token.isEmpty() &&
        !iriskit_settings.credential_token.equalsIgnoreCase("NULL") &&
        !iriskit_settings.server_address.isEmpty() &&
        !iriskit_settings.server_address.equalsIgnoreCase("NULL") &&
        !iriskit_settings.password.isEmpty() &&
        !iriskit_settings.password.equalsIgnoreCase("NULL")) {
        iris_kit_settings_loaded = true;
    }

    INFOF("iriskit_settings_loaded ? %s\n", iris_kit_settings_loaded ? "yes" : "no");

    // custom parameter for iris credentials
    WiFiManagerParameter* server_address = NULL;
    WiFiManagerParameter* credential_token = NULL;
    WiFiManagerParameter* password = NULL;

    memset(iris_server_address, 0, URL_SHORT_MAX);
    memset(iris_credential_token, 0, CREDENTIAL_MAX);
    memset(iris_password, 0, PASSWORD_MAX);

    if (iris_kit_settings_loaded) {
        INFOLN("iriskit settings loaded");
        strncpy(iris_server_address, iriskit_settings.server_address.c_str(), URL_SHORT_MAX - 1);
        strncpy(iris_credential_token, iriskit_settings.credential_token.c_str(), CREDENTIAL_MAX - 1);
        strncpy(iris_password, iriskit_settings.password.c_str(), PASSWORD_MAX - 1);
    }
    INFOLN("iriskit settings not loaded, set it from WifiManager");
    server_address =
        new WiFiManagerParameter("server_address", "Server Address", "iris.irext.net", URL_SHORT_MAX);
    credential_token =
        new WiFiManagerParameter("credential_token", "Credential Token", "", CREDENTIAL_MAX);
    password =
        new WiFiManagerParameter("password", "User Password", "", PASSWORD_MAX, "type='password'");

    if (NULL == server_address || NULL == credential_token || NULL == password) {
        ERRORLN("not enough memory to create settings");
        factoryReset();
    }
    wifi_manager.addParameter(server_address);
    wifi_manager.addParameter(credential_token);
    wifi_manager.addParameter(password);

    wifi_manager.autoConnect();

    if (!iris_kit_settings_loaded) {
        strncpy(iris_server_address, server_address->getValue(), URL_SHORT_MAX - 1);
        strncpy(iris_credential_token, credential_token->getValue(), CREDENTIAL_MAX - 1);
        String rawPassword = String(password->getValue());
        strncpy(iris_password, md5(rawPassword).c_str(), PASSWORD_MAX - 1);
    }

    // TODO: fix the logic without settings loaded
    INFOF("Wifi Connected, IRIS server = %s, credential token = %s, password = %s\n",
          iris_server_address, iris_credential_token, iris_password);

    do {
        if (WiFi.status() == WL_CONNECTED) {
            if (0 == authIrisKit(iris_credential_token,
                                 iris_password,
                                 g_product_key,
                                 g_device_name,
                                 g_device_secret,
                                 g_device_token,
                                 g_app_id)) {
                break;
            }
        }
        credential_init_retry++;
        if (credential_init_retry >= CREDENTIAL_INIT_RETRY_MAX) {
            ERRORLN("retried fetch credential for 3 times, reset WiFi");
            wifiReset();
        }
        delay(SYSTEM_DELAY);
    } while (1);

    INFOF("credential get : %s\n", iris_credential_token);
    iriskit_settings.server_address = String(iris_server_address);
    iriskit_settings.credential_token = String(iris_credential_token);
    iriskit_settings.password = String(iris_password);
    setIrisKitSettings(iriskit_settings);

    saveSettings();

    delay(SYSTEM_DELAY);
    uint8_t send_pin = ConfigData["pin"]["ir_send"];
    uint8_t recv_pin = ConfigData["pin"]["ir_receive"];
    if (send_pin == 0) {
        send_pin = ir_send_pin;
    }
    if (recv_pin == 0) {
        recv_pin = ir_receive_pin;
    }
    INFOF("IR pin config get : %d, %d\n", send_pin, recv_pin);
    loadIRPin(send_pin, recv_pin);

    // prepare MQTT connection params
    if (NULL != strstr(iris_server_address, "iris.irext.net")) {
        g_mqtt_server = String(MQTT_HOST_REL);
        g_runtime_env = RUNTIME_RELEASE;
    } else {
        String iris_server_address_dev = iriskit_settings.server_address;
        int delim = iris_server_address_dev.indexOf(':');
        if (delim != -1) {
            g_mqtt_server = iris_server_address_dev.substring(0, delim);
        } else {
            g_mqtt_server = iris_server_address_dev;
        }
        g_runtime_env = RUNTIME_DEBUG;
    }

    g_mqtt_client_id = g_device_name;
    g_mqtt_password = iriskit_settings.password;

    connectToIrextIoT();

    iotCheckTask.attach_scheduled(MQTT_CHECK_INTERVALS, checkIrisIoT);
    disableIRTask.attach_scheduled(DISABLE_SIGNAL_INTERVALS, disableIR);
}

void loop() {
    recvIR();
    irextIoTKeepAlive();
    yield();
}

void factoryReset() {
    static unsigned long last_interrupt_time = millis();
    unsigned long interrupt_time = millis();
    static unsigned long start_time = millis();
    unsigned long end_time = millis();
    if (interrupt_time - last_interrupt_time > 10) {
        start_time = millis();
    }
    last_interrupt_time = interrupt_time;
    if (end_time - start_time > 3000) {
        wifiReset();
    }
}

// private function defitions
static void wifiReset() {
    DEBUGLN("Reset settings");
    LittleFS.format();
    wifi_manager.resetSettings();
    WiFi.mode(WIFI_AP_STA); // cannot erase if not in STA mode !
    WiFi.persistent(true);
    WiFi.disconnect(true);
    WiFi.persistent(false);
    delay(SYSTEM_DELAY);
    ESP.reset();
}
