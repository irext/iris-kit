/**
 *
 * Copyright (c) 2020-2024 IRbaby-IRext
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

#ifndef IRIS_KIT_USER_SETTINGS_H
#define IRIS_KIT_USER_SETTINGS_H

#include <ArduinoJson.h>

#include "iris_kit.h"

#include "ir_ac_control.h"

/* save settings */
bool saveSettings();

/* load settings */
bool loadSettings();

bool saveACStatus(String file, t_remote_ac_status status);
t_remote_ac_status getACStatus(String file);

bool setIrisKitSettings(iris_kit_settings_t& iriskit_settings);
iris_kit_settings_t getIrisKitSettings();

extern StaticJsonDocument<1024> ConfigData;
extern StaticJsonDocument<1024> ACStatus;
extern StaticJsonDocument<1024> IrisKitSettings;

#endif // IRIS_KIT_USER_SETTINGS_H