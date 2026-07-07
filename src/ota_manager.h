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

#ifndef IRIS_KIT_OTA_MANAGER_H
#define IRIS_KIT_OTA_MANAGER_H

#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

// OTA upgrade status codes
typedef enum {
    OTA_STATUS_IDLE = 0,
    OTA_STATUS_DOWNLOADING = 1,
    OTA_STATUS_SUCCESS = 2,
    OTA_STATUS_FAILED = 3,
    OTA_STATUS_REBOOTING = 4,
} ota_status_t;

// Initialize OTA manager
void initOTA();

// Start firmware upgrade from URL
int startOTAUpgrade(const String& firmware_url, const String& target_version);

// Get current OTA status
ota_status_t getOTAStatus();

// Reset OTA status to idle
void resetOTAStatus();

// Report OTA status to cloud via MQTT
void reportOTAStatus(const String& message);

#endif // IRIS_KIT_OTA_MANAGER_H
