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

#include <Ticker.h>
#include <Arduino.h>
#include <ESP8266HTTPClient.h>

#include "defines.h"
#include "IRbabyIR.h"
#include "IRbabyOTA.h"

#if defined USE_IRBABY_MQTT
    #include "IRbabyMQTT.h"
#else
    #include "IRbabyAlink.h"
#endif

#include "IRbabyMsgHandler.h"
#include "IRbabyGlobal.h"
#include "IRbabyUserSettings.h"
#include "IRbabyHttp.h"
#include "IRbabyIRIS.h"
#include "IRbabyRF.h"

#include "IRbaby.h"

#define CREDENTIAL_INIT_RETRY_MAX  (3)

extern char iris_server_address[];
extern char iris_credential_token[];

extern String g_product_key;
extern String g_device_name;
extern String g_device_secret;

int credential_init_retry = 0;


void uploadIP();                       // device info upload to devicehive
void IRAM_ATTR resetHandle();          // interrupt handle

#if defined USE_IRBABY_MQTT
    Ticker mqttCheckTask;              // MQTT check timer
#else
    Ticker alinkCheckTask;             // Aliyun IoT MQTT check timer
#endif

Ticker disableIRTask;                  // disable IR receive
Ticker disableRFTask;                  // disable RF receive
Ticker saveDataTask;                   // save data

// LSOC DAS2 related tasks
Ticker lsocHeartBeatTask;              // lsoc heart beat sync

void setup() {
    if (LOG_DEBUG || LOG_ERROR || LOG_INFO) {
        Serial.begin(BAUD_RATE);
    }

    pinMode(RESET_PIN, INPUT_PULLUP);
    pinMode(0, OUTPUT);
    digitalWrite(0, LOW);
    attachInterrupt(digitalPinToInterrupt(RESET_PIN), resetHandle, ONLOW);

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

    WiFiManagerParameter server_address("server", "Server", "http://192.168.2.31:8081", URL_SHORT_MAX);
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
        if(WiFi.status()== WL_CONNECTED) {
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
            wifiReset();
        }
        delay(1000);
    } while (1);

    INFOF("credential get : %s\n", iris_credential_token);

    settingsLoad(); // load user settings form fs
    delay(1000);

    connectToAliyunIoT();
#ifdef USE_RF
    initRF(); // RF init
#endif
    loadIRPin(ConfigData["pin"]["ir_send"], ConfigData["pin"]["ir_receive"]);

#if defined USE_IRBABY_MQTT
    mqttCheckTask.attach_scheduled(MQTT_CHECK_INTERVALS, mqttCheck);
#else
    alinkCheckTask.attach_scheduled(MQTT_CHECK_INTERVALS, checkAlinkMQTT);
#endif
    disableIRTask.attach_scheduled(DISABLE_SIGNAL_INTERVALS, disableIR);
    disableRFTask.attach_scheduled(DISABLE_SIGNAL_INTERVALS, disableRF);
    saveDataTask.attach_scheduled(SAVE_DATA_INTERVALS, settingsSave);
}

void wifiReset() {
    WiFi.disconnect();
    ESP.reset();
}

void loop() {
    /* IR receive */
    recvIR();
#ifdef USE_RF
    /* RF receive */
    recvRF();
#endif

#if 0
    /* UDP receive and handle */
    char *msg = udpRecive();
    if (msg) {
        udp_msg_doc.clear();
        DeserializationError error = deserializeJson(udp_msg_doc, msg);
        if (error) {
            ERRORLN("Failed to parse udp message");
        }
        msgHandle(&udp_msg_doc, MsgType::udp);
    }
#endif

#if defined USE_IRBABY_MQTT
    /* MQTT loop */
    mqttLoop();
#endif

    yield();
}

void resetHandle() {
    static unsigned long last_interrupt_time = millis();
    unsigned long interrupt_time = millis();
    static unsigned long start_time = millis();
    unsigned long end_time = millis();
    if (interrupt_time - last_interrupt_time > 10) {
        start_time = millis();
    }
    last_interrupt_time = interrupt_time;
    if (end_time - start_time > 3000) {
        settingsClear();
    }
}