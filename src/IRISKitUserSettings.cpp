/**
 *
 * Copyright (c) 2020-2022 IRbaby-IRext
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
#include "IRISKitGlobal.h"
#include "IRISKitSerials.h"
#include "IRISKitIR.h"

#include "IRISKitUserSettings.h"


#define FILE_GENERIC_CONFIG      "config"
#define FILE_AC_STATUS           "ac_status"
#define IRISKIT_SETTINGS         "iriskit_settings"


StaticJsonDocument<1024> ConfigData;
StaticJsonDocument<1024> ACStatus;
StaticJsonDocument<1024> IrisKitSettings;

bool saveSettings() {
    DEBUGLN("save configs for IRIS Kit");

    // generic config partial
    File cache = LittleFS.open(FILE_GENERIC_CONFIG, "w");
    if (!cache || (serializeJson(ConfigData, cache) == 0)) {
        ERRORLN("failed to save config file");
        cache.close();
        return false;
    }
    cache.close();

    // AC status partial
    cache = LittleFS.open(FILE_AC_STATUS, "w");
    if (!cache || (serializeJson(ACStatus, cache) == 0)) {
        ERRORLN("ERROR: failed to save AC status file");
        cache.close();
        return false;
    }
    cache.close();

    // credential partial
    cache = LittleFS.open(IRISKIT_SETTINGS, "w");
    if (!cache || (serializeJson(IrisKitSettings, cache) == 0)) {
        ERRORLN("ERROR: failed to save credentials file");
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
    DEBUGF("fsstats: total bytes = %llu, used = %llu\n", info.totalBytes, info.usedBytes);

    // generic config partial
    if (LittleFS.exists(FILE_GENERIC_CONFIG)) {
        File cache = LittleFS.open(FILE_GENERIC_CONFIG, "r");
        if (!cache) {
            ERRORLN("failed to read config file");
            return ret;
        }
        if (cache.size() > 0) {
            DeserializationError error = deserializeJson(ConfigData, cache);
            if (error) {
                ERRORLN("failed to load config settings");
                return ret;
            }
            INFOLN("generic config loaded");
            ConfigData["version"] = FIRMWARE_VERSION;
            serializeJsonPretty(ConfigData, Serial);
            Serial.println();
        }
        cache.close();
    } else {
        DEBUGLN("config does not exist");
    }

    // AC status partial
    if (LittleFS.exists(FILE_AC_STATUS)) {
        File cache = LittleFS.open(FILE_AC_STATUS, "r");
        if (!cache) {
            ERRORLN("failed to read AC status file");
            return ret;
        }
        if (cache.size() > 0) {
            DeserializationError error = deserializeJson(ACStatus, cache);
            if (error) {
                ERRORLN("failed to load AC status settings");
                return ret;
            }
            INFOLN("AC status loaded");
        }
        cache.close();
    }

    // credential partial
    if (LittleFS.exists(IRISKIT_SETTINGS)) {
        File cache = LittleFS.open(IRISKIT_SETTINGS, "r");
        if (!cache) {
            ERRORLN("failed to read acstatus file");
            return ret;
        }
        if (cache.size() > 0) {
            DeserializationError error = deserializeJson(IrisKitSettings, cache);
            if (error) {
                ERRORLN("failed to load credentials settings");
                return ret;
            }
            INFOLN("credentials loaded");
        }
        cache.close();
    }

    ret = true;
    return ret;
}

bool saveACStatus(String file, t_remote_ac_status status) {
    bool ret = true;
    ACStatus[file]["power"] = (int)status.ac_power;
    ACStatus[file]["temperature"] = (int)status.ac_temp;
    ACStatus[file]["mode"] = (int)status.ac_mode;
    ACStatus[file]["swing"] = (int)status.ac_swing;
    ACStatus[file]["speed"] = (int)status.ac_wind_speed;
    return ret;
}

t_remote_ac_status getACStatus(String file) {
    t_remote_ac_status status;
    int power = (int) ACStatus[file]["power"];
    int temperature = (int) ACStatus[file]["temperature"];
    int mode = (int) ACStatus[file]["mode"];
    int swing = (int) ACStatus[file]["swing"];
    int wind_speed = (int) ACStatus[file]["speed"];
    status.ac_power = (t_ac_power)power;
    status.ac_temp = (t_ac_temperature)temperature;
    status.ac_mode = (t_ac_mode)mode;
    status.ac_swing = (t_ac_swing)swing;
    status.ac_wind_speed = (t_ac_wind_speed)wind_speed;
    return status;
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