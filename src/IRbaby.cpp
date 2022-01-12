/**
 *
 * Copyright (c) 2020-2021 IRbaby-IRext
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
#include "IRbabyUDP.h"
#include "IRbabyOTA.h"

#if defined USE_IRBABY_MQTT
    #include "IRbabyMQTT.h"
#else
    #include "IRbabyAlink.h"
#endif

#include "IRbabyMsgHandler.h"
#include "IRbabyGlobal.h"
#include "IRbabyUserSettings.h"
#include "IRbabyRF.h"

#include "IRbabySecurity.h"

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
    INFOLN("== IRIS Kit [0.2.7] Powered by IRBaby ==");

    wifi_manager.autoConnect();
    settingsLoad(); // load user settings form fs
    delay(5);
    udpInit();
    connectToAliyunIoT();
#ifdef USE_RF
    initRF(); // RF init
#endif
    loadIRPin(ConfigData["pin"]["ir_send"], ConfigData["pin"]["ir_receive"]);
#ifdef USE_INFO_UPLOAD
    uploadIP();
#endif

#if defined USE_IRBABY_MQTT
    mqttCheckTask.attach_scheduled(MQTT_CHECK_INTERVALS, mqttCheck);
#else
    alinkCheckTask.attach_scheduled(MQTT_CHECK_INTERVALS, checkAlinkMQTT);
#endif
    disableIRTask.attach_scheduled(DISABLE_SIGNAL_INTERVALS, disableIR);
    disableRFTask.attach_scheduled(DISABLE_SIGNAL_INTERVALS, disableRF);
    saveDataTask.attach_scheduled(SAVE_DATA_INTERVALS, settingsSave);

    // LSOC DAS2 related block
    lsocHeartBeatTask.attach_scheduled(LSOC_HB_CYCLE, lsocInfoReport);
}

void loop() {
    /* IR receive */
    recvIR();
#ifdef USE_RF
    /* RF receive */
    recvRF();
#endif
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

// only upload chip id
void uploadIP() {
    HTTPClient http;
    StaticJsonDocument<128> body_json;
    String chip_id = String(ESP.getChipId(), HEX);
    chip_id.toUpperCase();
    String head = "http://playground.devicehive.com/api/rest/device/";
    head += chip_id;
    http.begin(wifi_client, head);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization",
                   "Bearer eyJhbGciOiJIUzI1NiJ9.eyJ"
                   "wYXlsb2FkIjp7ImEiOlsyLDMsNCw1LD"
                   "YsNyw4LDksMTAsMTEsMTIsMTUsMTYsM"
                   "TddLCJlIjoxNzQzNDM2ODAwMDAwLCJ0"
                   "IjoxLCJ1Ijo2NjM1LCJuIjpbIjY1NDI"
                   "iXSwiZHQiOlsiKiJdfX0.WyyxNr2OD5"
                   "pvBSxMq84NZh6TkNnFZe_PXenkrUkRS"
                   "iw");
    body_json["name"] = chip_id;
    body_json["networkId"] = "6542";
    String body = body_json.as<String>();
    INFOF("update %s to devicehive\n", body.c_str());
    http.PUT(body);
    http.end();
}

// security related
int securityPublish(const char *topic, const uint8_t *message, size_t msg_size, void *channel) {
#if defined USE_IRBABY_MQTT 
    // message via MQTT instance(not implemented)
#else
    // message via embedded Alink instance
    sendRawData(topic, message, msg_size);
#endif
    return 0;
}