/**
 *
 * Copyright (c) 2020-2025 IRext Opensource Organization
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
#include "ir_drv_ctrl.h"
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
constexpr unsigned long utcOffsetInMilliSeconds = 3600 * 1000;

int credential_init_retry = 0;
int g_runtime_env = RUNTIME_RELEASE;
iris_kit_settings_t iris_kit_settings;
bool iris_kit_settings_loaded = false;
iris_kit_status_t g_iris_kit_status = {
    .console_id = 0,
    .remote_index = "",
    .key_id = 0,
    .key_name = "",
    .status = IRIS_KIT_STATUS_IDLE
};


// private variable definitions
static Ticker iot_check_task;            // IoT MQTT check timer
static Ticker disable_ir_task;           // disable IR receive


// private function declarations
static void wifiReset();

// public function definitions
void setup() {
    if constexpr (LOG_DEBUG || LOG_ERROR || LOG_INFO) {
        Serial.begin(BAUD_RATE);
    }

    pinMode(RESET_PIN, INPUT_PULLUP);
    pinMode(0, OUTPUT);
    digitalWrite(0, LOW);
    attachInterrupt(digitalPinToInterrupt(RESET_PIN), factoryReset, ONLOW);
    delay(SYSTEM_DELAY);

    Serial.clearWriteError();
    Serial.print("\n");
    Serial.print("██╗██████╗ ██╗███████╗\n");
    Serial.print("██║██╔══██╗██║██╔════╝\n");
    Serial.print("██║██████╔╝██║███████╗\n");
    Serial.print("██║██╔══██╗██║╚════██║\n");
    Serial.print("██║██║  ██║██║███████║\n");
    Serial.print("╚═╝╚═╝  ╚═╝╚═╝╚══════╝\n");
    Serial.print("== IRIS Kit [1.4.0_r1] Powered by AliyunIoT ==\n");

    // try loading saved iriskit settings
    iris_kit_settings.credential_token.clear();
    iris_kit_settings.server_address.clear();
    iris_kit_settings.password.clear();

    if (loadSettings()) {
        iris_kit_settings = getIrisKitSettings();
        INFOF("Saved credentials loaded\n");
        INFOF("Server address is empty ? %s\n", iris_kit_settings.server_address.isEmpty() ? "yes" : "no");
        INFOF("Credential is empty ? %s\n", iris_kit_settings.credential_token.isEmpty() ? "yes" : "no");
        INFOF("Password is empty ? %s\n", iris_kit_settings.password.isEmpty() ? "yes" : "no");
    }
    if (!iris_kit_settings.credential_token.isEmpty() &&
        !iris_kit_settings.credential_token.equalsIgnoreCase("nullptr") &&
        !iris_kit_settings.server_address.isEmpty() &&
        !iris_kit_settings.server_address.equalsIgnoreCase("nullptr") &&
        !iris_kit_settings.password.isEmpty() &&
        !iris_kit_settings.password.equalsIgnoreCase("nullptr")) {
        iris_kit_settings_loaded = true;
    }

    INFOF("Setting loaded = %s\n", iris_kit_settings_loaded ? "successfully" : "failed");

    // custom parameter for iris credentials
    WiFiManagerParameter* server_address = nullptr;
    WiFiManagerParameter* credential_token = nullptr;
    WiFiManagerParameter* password = nullptr;

    memset(iris_server_address, 0, URL_SHORT_MAX);
    memset(iris_credential_token, 0, CREDENTIAL_MAX);
    memset(iris_password, 0, PASSWORD_MAX);

    if (iris_kit_settings_loaded) {
        INFOF("Settings loaded\n");
        strncpy(iris_server_address, iris_kit_settings.server_address.c_str(), URL_SHORT_MAX - 1);
        strncpy(iris_credential_token, iris_kit_settings.credential_token.c_str(), CREDENTIAL_MAX - 1);
        strncpy(iris_password, iris_kit_settings.password.c_str(), PASSWORD_MAX - 1);
    }
    INFOF("Settings not loaded, set it from WifiManager\n");
    server_address =
        new WiFiManagerParameter("server_address", "Server Address", "iris.irext.net", URL_SHORT_MAX);
    credential_token =
        new WiFiManagerParameter("credential_token", "Credential Token", "", CREDENTIAL_MAX);
    password =
        new WiFiManagerParameter("password", "User Password", "", PASSWORD_MAX, "type='password'");

    if (nullptr == server_address || nullptr == credential_token || nullptr == password) {
        ERRORF("Not enough memory to create settings\n");
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
            ERRORF("Retried fetch credential for 3 times, reset WiFi\n");
            wifiReset();
        }
        delay(AUTH_RETRY_DELAY);
    } while (1);

    INFOF("Credential get : %s\n", iris_credential_token);
    iris_kit_settings.server_address = String(iris_server_address);
    iris_kit_settings.credential_token = String(iris_credential_token);
    iris_kit_settings.password = String(iris_password);
    setIrisKitSettings(iris_kit_settings);

    resetIrisKitStatus();

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
    if (nullptr != strstr(iris_server_address, "iris.irext.net")) {
        g_mqtt_server = String(MQTT_HOST_REL);
        g_runtime_env = RUNTIME_RELEASE;
    } else {
        String iris_server_address_dev = iris_kit_settings.server_address;
        int delim = iris_server_address_dev.indexOf(':');
        if (delim != -1) {
            g_mqtt_server = iris_server_address_dev.substring(0, delim);
        } else {
            g_mqtt_server = iris_server_address_dev;
        }
        g_runtime_env = RUNTIME_DEBUG;
    }

    g_mqtt_client_id = g_device_name;
    g_mqtt_password = iris_kit_settings.password;

    if (0 != connectIot()) {
        INFOF("Failed to connect IoT at startup\n");
    }

    iot_check_task.attach_scheduled(MQTT_CHECK_INTERVALS, checkIot);
    disable_ir_task.attach_scheduled(DISABLE_SIGNAL_INTERVALS, disableIRIn);
}

void loop() {
    // process unsolicited FSM state change
    if (IRIS_KIT_STATUS_READY_TO_STUDY == g_iris_kit_status.status) {
        recvIR();
    } else if (IRIS_KIT_STATUS_UPLOADED == g_iris_kit_status.status) {
        resetIrisKitStatus();
    }
    keepAliveIot();
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

// private function definitions
static void wifiReset() {
    INFOF("Reset settings\n");
    LittleFS.format();
    wifi_manager.resetSettings();
    WiFi.mode(WIFI_AP_STA); // cannot erase if not in STA mode !
    WiFi.persistent(true);
    WiFi.disconnect(true);
    WiFi.persistent(false);
    delay(SYSTEM_DELAY);
    ESP.reset();
}
