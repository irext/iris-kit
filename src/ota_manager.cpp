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
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <Esp.h>

#include "defines.h"
#include "serials.h"
#include "global.h"
#include "iot_hub.h"
#include "iris_client.h"

#include "ota_manager.h"

// external variable declarations
extern String g_product_key;
extern String g_device_name;
extern String g_upstream_topic;
extern int g_app_id;

// private variable definitions
static ota_status_t current_ota_status = OTA_STATUS_IDLE;
static String pending_firmware_url = "";
static String pending_version = "";
static int current_download_percent = 0;

// private function declarations
static void httpUpdateStarted();
static void httpUpdateProgress(int current, int total);
static void httpUpdateFinished();

// public function definitions
void initOTA() {
    INFOF("OTA Manager initialized\n");
    
    // Set up HTTP update callbacks
    ESPhttpUpdate.onStart(httpUpdateStarted);
    ESPhttpUpdate.onProgress(httpUpdateProgress);
    ESPhttpUpdate.onEnd(httpUpdateFinished);
    
    current_ota_status = OTA_STATUS_IDLE;
}

int startOTAUpgrade(const String& firmware_url, const String& target_version) {
    if (firmware_url.isEmpty()) {
        ERRORF("Firmware URL is empty\n");
        return -1;
    }
    
    // Check if already upgrading
    if (current_ota_status == OTA_STATUS_DOWNLOADING || 
        current_ota_status == OTA_STATUS_REBOOTING) {
        ERRORF("OTA upgrade already in progress\n");
        return -1;
    }
    
    INFOF("Starting OTA upgrade from: %s\n", firmware_url.c_str());
    INFOF("Target version: %s\n", target_version.c_str());
    
    // Store pending upgrade info
    pending_firmware_url = firmware_url;
    pending_version = target_version;
    
    // Update status
    current_ota_status = OTA_STATUS_DOWNLOADING;
    reportOTAStatus(OTA_STATUS_DOWNLOADING, "Downloading firmware...");
    
    INFOF("Starting HTTP update with WiFi client...\n");
    
    // Start HTTP update
    t_httpUpdate_return ret = ESPhttpUpdate.update(wifi_client, firmware_url);
    
    INFOF("HTTP update returned: %d\n", ret);
    
    if (ret == HTTP_UPDATE_FAILED) {
        int error_code = ESPhttpUpdate.getLastError();
        String error_str = ESPhttpUpdate.getLastErrorString();
        ERRORF("HTTP_UPDATE_FAILED: (%d) %s\n", error_code, error_str.c_str());
        current_ota_status = OTA_STATUS_FAILED;
        // Note: Do NOT report via MQTT - connection may be broken
        // Reboot to ensure clean state, server will detect failure via unchanged version
        delay(500);
        current_ota_status = OTA_STATUS_REBOOTING;
        ESP.restart();
        return -1;
    }
    
    if (ret == HTTP_UPDATE_NO_UPDATES) {
        INFOF("HTTP_UPDATE_NO_UPDATES - firmware is same or older\n");
        current_ota_status = OTA_STATUS_IDLE;
        // Note: Do NOT report in MQTT - connection may be broken
        return 0;
    }
    
    if (ret == HTTP_UPDATE_OK) {
        INFOF("HTTP_UPDATE_OK - rebooting...\n");
        current_ota_status = OTA_STATUS_SUCCESS;
        // Note: Do NOT report in MQTT - will reboot immediately
        delay(500);
        current_ota_status = OTA_STATUS_REBOOTING;
        ESP.restart();
        return 0;
    }
    
    return -1;
}

ota_status_t getOTAStatus() {
    return current_ota_status;
}

void resetOTAStatus() {
    current_ota_status = OTA_STATUS_IDLE;
    current_download_percent = 0;
}

void reportOTAStatus(ota_status_t status, const String& message) {
    INFOF("OTA Status: %d, Message: %s\n", status, message.c_str());

    StaticJsonDocument<512> doc;

    String stage;
    bool success = true;

    switch (status) {
        case OTA_STATUS_IDLE:
            stage = "idle";
            break;
        case OTA_STATUS_DOWNLOADING:
            stage = "downloading";
            break;
        case OTA_STATUS_SUCCESS:
            stage = "completed";
            break;
        case OTA_STATUS_FAILED:
            stage = "failed";
            success = false;
            break;
        case OTA_STATUS_REBOOTING:
            stage = "rebooting";
            break;
    }

    doc["eventName"] = "__firmware_update_progress";
    doc["productKey"] = g_product_key;
    doc["deviceName"] = g_device_name;
    doc["appId"] = String(g_app_id);
    doc["consoleId"] = "0";
    doc["stage"] = stage;
    doc["message"] = message;
    doc["success"] = success;

    String mqtt_message;
    serializeJson(doc, mqtt_message);
    sendData(g_upstream_topic.c_str(), (uint8_t*)mqtt_message.c_str(), mqtt_message.length());
}

// private function definitions
static void httpUpdateStarted() {
    INFOF("HTTP update started\n");
}

static void httpUpdateProgress(int current, int total) {
    int percent = (total > 0) ? (current * 100 / total) : 0;
    
    // Only log locally during download to avoid MQTT issues
    if (percent % 10 == 0 && percent != current_download_percent) {
        current_download_percent = percent;
        INFOF("HTTP update progress: %d%% (%d/%d bytes)\n", percent, current, total);
    }
}

static void httpUpdateFinished() {
    INFOF("HTTP update finished\n");
}
