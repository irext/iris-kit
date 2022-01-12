/**
 *
 * Copyright (c) 2020-2021 IRbaby-IRext
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

#include "IRbabyUserSettings.h"
#include "IRbabySerial.h"
#include "IRbabyMQTT.h"
#include "WiFiManager.h"
#include "IRbabyGlobal.h"
#include "IRbabyIR.h"
#include "defines.h"

StaticJsonDocument<1024> ConfigData;
StaticJsonDocument<1024> ACStatus;

bool settingsSave()
{
    DEBUGLN("Save Config");
    File cache = LittleFS.open("/config", "w");
    if (!cache || (serializeJson(ConfigData, cache) == 0)) {
        ERRORLN("Failed to save config file");
        cache.close();
        return false;
    }
    cache.close();
    cache = LittleFS.open("/acstatus", "w");
    if (!cache || (serializeJson(ACStatus, cache) == 0))
    {
        ERRORLN("ERROR: Failed to save acstatus file");
        cache.close();
        return false;        
    }
    cache.close();
    return true;
}

bool settingsLoad()
{
    LittleFS.begin();
    int ret = false;
    FSInfo64 info;
    LittleFS.info64(info);
    DEBUGF("fs total bytes = %llu\n", info.totalBytes);
    if (LittleFS.exists("/config"))
    {
        File cache = LittleFS.open("/config", "r");
        if (!cache)
        {
            ERRORLN("Failed to read config file");
            return ret;
        }
        if (cache.size() > 0)
        {
            DeserializationError error = deserializeJson(ConfigData, cache);
            if (error)
            {
                ERRORLN("Failed to load config settings");
                return ret;
            }
            INFOLN("Load config data:");
            ConfigData["version"] = FIRMWARE_VERSION;
            serializeJsonPretty(ConfigData, Serial);
            Serial.println();
        }
        cache.close();
    } else {
        DEBUGLN("Config does not exist");
    }

    if (LittleFS.exists("/acstatus")) {
        File cache = LittleFS.open("/acstatus", "r");
        if (!cache) {
            ERRORLN("Failed to read acstatus file");
            return ret;
        }
        if (cache.size() > 0) {
            DeserializationError error = deserializeJson(ACStatus, cache);
            if (error) {
                ERRORLN("Failed to load acstatus settings");
                return ret;
            }
        }
        cache.close();
    }
    ret = true;
    return ret;
}

void settingsClear()
{
    DEBUGLN("\nReset settings");
    wifi_manager.resetSettings();
    LittleFS.format();
    ESP.reset();
}

bool saveACStatus(String file, t_remote_ac_status status)
{
    bool ret = false;
    ACStatus[file]["power"] = (int)status.ac_power;
    ACStatus[file]["temperature"] = (int)status.ac_temp;
    ACStatus[file]["mode"] = (int)status.ac_mode;
    ACStatus[file]["swing"] = (int)status.ac_swing;
    ACStatus[file]["speed"] = (int)status.ac_wind_speed;
    return ret;
}

t_remote_ac_status getACState(String file)
{
    t_remote_ac_status status;
    int power = (int)ACStatus[file]["power"];
    int temperature = (int)ACStatus[file]["temperature"];
    int mode = (int)ACStatus[file]["mode"];
    int swing = (int)ACStatus[file]["swing"];
    int wind_speed = (int)ACStatus[file]["speed"];
    status.ac_power = (t_ac_power)power;
    status.ac_temp = (t_ac_temperature)temperature;
    status.ac_mode = (t_ac_mode)mode;
    status.ac_swing = (t_ac_swing)swing;
    status.ac_wind_speed = (t_ac_wind_speed)wind_speed;
    return status;
}
