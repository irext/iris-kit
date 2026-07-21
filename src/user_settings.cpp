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

#include <LittleFS.h>
#include <WiFiManager.h>

#include "defines.h"
#include "global.h"
#include "serials.h"

#include "user_settings.h"


#define FILE_GENERIC_CONFIG      "config"
#define IRIS_KIT_SETTINGS        "iriskit_settings"


StaticJsonDocument<1024> ConfigData;
StaticJsonDocument<1024> IrisKitSettings;

bool saveSettings() {
    INFOF("Save configs\n");

    // generic config partial
    File cache = LittleFS.open(FILE_GENERIC_CONFIG, "w");
    if (!cache || (serializeJson(ConfigData, cache) == 0)) {
        ERRORF("Failed to save config file\n");
        cache.close();
        return false;
    }
    cache.close();

    // credential partial
    cache = LittleFS.open(IRIS_KIT_SETTINGS, "w");
    if (!cache || (serializeJson(IrisKitSettings, cache) == 0)) {
        ERRORF("ERROR: failed to save iriskit_settings file\n");
        cache.close();
        return false;
    }
    cache.close();
    return true;
}

bool loadSettings() {
    LittleFS.begin();
    int ret = false;
    FSInfo64 info;
    LittleFS.info64(info);
    INFOF("Load settings, total bytes = %llu, used = %llu\n", info.totalBytes, info.usedBytes);

    // generic config partial
    if (LittleFS.exists(FILE_GENERIC_CONFIG)) {
        File cache = LittleFS.open(FILE_GENERIC_CONFIG, "r");
        if (!cache) {
            ERRORF("Failed to read config file\n");
            return ret;
        }
        if (cache.size() > 0) {
            DeserializationError error = deserializeJson(ConfigData, cache);
            if (error) {
                ERRORF("Failed to load config settings\n");
                return ret;
            }
            INFOF("Generic config loaded\n");
            ConfigData["version"] = FIRMWARE_VERSION;
            serializeJsonPretty(ConfigData, Serial);
            Serial.println();
        }
        cache.close();
    } else {
        ERRORF("Config does not exist\n");
    }

    // credential partial
    if (LittleFS.exists(IRIS_KIT_SETTINGS)) {
        File cache = LittleFS.open(IRIS_KIT_SETTINGS, "r");
        if (!cache) {
            ERRORF("Failed to read iriskit_settings file\n");
            return ret;
        }
        if (cache.size() > 0) {
            DeserializationError error = deserializeJson(IrisKitSettings, cache);
            if (error) {
                ERRORF("Failed to load credentials settings\n");
                return ret;
            }
            INFOF("Credentials loaded\n");
        }
        cache.close();
    }

    ret = true;
    return ret;
}

bool setIrisKitSettings(iris_kit_settings_t& iriskit_settings) {
    IrisKitSettings["server_address"] = iriskit_settings.server_address;
    IrisKitSettings["token"] = iriskit_settings.credential_token;
    IrisKitSettings["password"] = iriskit_settings.password;
    return true;
}

iris_kit_settings_t getIrisKitSettings() {
    iris_kit_settings_t iriskit_settings;
    iriskit_settings.server_address = (String)IrisKitSettings["server_address"];
    iriskit_settings.credential_token = (String)IrisKitSettings["token"];
    iriskit_settings.password = (String)IrisKitSettings["password"];
    return iriskit_settings;
}