/**
 *
 * Copyright (c) 2020-2022 IRbaby-IRext
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

#include "defines.h"
#include "IRbabyIR.h"
#include "IRbabyAlink.h"
#include "IRbabyHttp.h"
#include "IRbabyGlobal.h"
#include "IRbabyIRIS.h"
#include "IRbabySerial.h"
#include "IRbabyUserSettings.h"

#include "IRbaby.h"

#define CREDENTIAL_INIT_RETRY_MAX  (3)

// external variable declarations
extern char iris_server_address[];
extern char iris_credential_token[];

extern String g_product_key;
extern String g_device_name;
extern String g_device_secret;


// public variable definitions
int credential_init_retry = 0;



// private variable definitions
static Ticker alinkCheckTask;          // Aliyun IoT MQTT check timer
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

    delay(10);

    Serial.clearWriteError();
    INFOLN();
    INFOLN("██╗██████╗ ██╗███████╗");
    INFOLN("██║██╔══██╗██║██╔════╝");
    INFOLN("██║██████╔╝██║███████╗");
    INFOLN("██║██╔══██╗██║╚════██║");
    INFOLN("██║██║  ██║██║███████║");
    INFOLN("╚═╝╚═╝  ╚═╝╚═╝╚══════╝");
    INFOLN("== IRIS Kit [1.2.7] Powered by IRBaby ==");

    // custom parameter for iris credentials
    WiFiManagerParameter server_address("server", "Server", "", URL_SHORT_MAX);
    WiFiManagerParameter credential_token("credential", "Credential", "", CREDENTIAL_MAX);

    wifi_manager.addParameter(&server_address);
    wifi_manager.addParameter(&credential_token);
    wifi_manager.autoConnect();

    memset(iris_server_address, 0, URL_SHORT_MAX);
    strcpy(iris_server_address, server_address.getValue());

    memset(iris_credential_token, 0, CREDENTIAL_MAX);
    strcpy(iris_credential_token, credential_token.getValue());

    INFOF("Wifi Connected, IRIS server = %s, credential token = %s\n",
          iris_server_address, iris_credential_token);

    do {
        if (WiFi.status() == WL_CONNECTED) {
            if (0 == fetchIrisCredential(iris_credential_token,
                                         g_product_key,
                                         g_device_name,
                                         g_device_secret)) {
                break;
            }
        }
        credential_init_retry++;
        if (credential_init_retry >= CREDENTIAL_INIT_RETRY_MAX) {
            ERRORLN("retried fetch credential for 3 times, reset WiFi");
            factoryReset();
        }
        delay(1000);
    } while (1);

    INFOF("credential get : %s\n", iris_credential_token);

    delay(1000);
    connectToAliyunIoT();
    loadIRPin(ConfigData["pin"]["ir_send"], ConfigData["pin"]["ir_receive"]);

    alinkCheckTask.attach_scheduled(MQTT_CHECK_INTERVALS, checkAlinkMQTT);
    disableIRTask.attach_scheduled(DISABLE_SIGNAL_INTERVALS, disableIR);
}

void loop() {
    recvIR();
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
        factoryReset();
    }
}



// private function defitions
static void wifiReset() {
    DEBUGLN("\nReset settings");
    wifi_manager.resetSettings();
    LittleFS.format();
    ESP.reset();
}
