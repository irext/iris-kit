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

// private variable definitions
static ota_status_t current_ota_status = OTA_STATUS_IDLE;
static ota_callback_t ota_callback = nullptr;
static String pending_firmware_url = "";
static String pending_version = "";
static int pending_version_code = 0;

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

int startOTAUpgrade(const String& firmware_url, const String& target_version, int target_version_code) {
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
    INFOF("Target version: %s (code: %d)\n", target_version.c_str(), target_version_code);
    
    // Store pending upgrade info
    pending_firmware_url = firmware_url;
    pending_version = target_version;
    pending_version_code = target_version_code;
    
    // Update status
    current_ota_status = OTA_STATUS_DOWNLOADING;
    reportOTAStatus(OTA_STATUS_DOWNLOADING, "Downloading firmware...");
    
    // Start HTTP update
    t_httpUpdate_return ret = ESPhttpUpdate.update(wifi_client, firmware_url);
    
    INFOF("HTTP update returned: %d\n", ret);
    
    switch (ret) {
        case HTTP_UPDATE_FAILED:
            ERRORF("HTTP_UPDATE_FAILED: (%d) %s\n", 
                   ESPhttpUpdate.getLastError(), 
                   ESPhttpUpdate.getLastErrorString().c_str());
            current_ota_status = OTA_STATUS_FAILED;
            reportOTAStatus(OTA_STATUS_FAILED, ESPhttpUpdate.getLastErrorString().c_str());
            return -1;
            
        case HTTP_UPDATE_NO_UPDATES:
            INFOF("HTTP_UPDATE_NO_UPDATES - firmware is same or older\n");
            current_ota_status = OTA_STATUS_IDLE;
            reportOTAStatus(OTA_STATUS_IDLE, "No updates available");
            return 0;
            
        case HTTP_UPDATE_OK:
            INFOF("HTTP_UPDATE_OK - rebooting...\n");
            current_ota_status = OTA_STATUS_SUCCESS;
            reportOTAStatus(OTA_STATUS_SUCCESS, "Update successful, rebooting...");
            
            // Delay to allow MQTT message to be sent
            delay(1000);
            
            current_ota_status = OTA_STATUS_REBOOTING;
            ESP.restart();
            return 0;
    }
    
    return -1;
}

ota_status_t getOTAStatus() {
    return current_ota_status;
}

void setOTACallback(ota_callback_t callback) {
    ota_callback = callback;
}

void reportOTAStatus(ota_status_t status, const String& message) {
    INFOF("OTA Status: %d, Message: %s\n", status, message.c_str());
    
    // Call user callback if set
    if (ota_callback != nullptr) {
        ota_callback(status, message.c_str());
    }
    
    // TODO: Send status to cloud via MQTT upstream topic
    // This will be implemented after integrating with iris_client
}

// private function definitions
static void httpUpdateStarted() {
    INFOF("HTTP update started\n");
    reportOTAStatus(OTA_STATUS_DOWNLOADING, "Download started");
}

static void httpUpdateProgress(int current, int total) {
    static int last_percent = -1;
    int percent = (total > 0) ? (current * 100 / total) : 0;
    
    // Only log every 10% to avoid spam
    if (percent % 10 == 0 && percent != last_percent) {
        INFOF("HTTP update progress: %d%% (%d/%d bytes)\n", percent, current, total);
        last_percent = percent;
    }
}

static void httpUpdateFinished() {
    INFOF("HTTP update finished\n");
}
