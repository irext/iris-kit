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

#include "IRbabyOTA.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266WiFi.h>

void update_started() {
    DEBUGLN("CALLBACK:  HTTP update process started");
}

void update_finished() {
    DEBUGLN("CALLBACK:  HTTP update process finished");
}

void update_progress(int cur, int total) {
    DEBUGF("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
}

void update_error(int err) {
    DEBUGF("CALLBACK:  HTTP update fatal error code %d\n", err);
}

void versionCheck() {
}

void otaUpdate(String url) {

    WiFiClient client;
    // Add optional callback notifiers
    ESPhttpUpdate.onStart(update_started);
    ESPhttpUpdate.onEnd(update_finished);
    ESPhttpUpdate.onProgress(update_progress);
    ESPhttpUpdate.onError(update_error);

    t_httpUpdate_return ret = ESPhttpUpdate.update(client, url);

    switch (ret) {
        case HTTP_UPDATE_FAILED:
            DEBUGF("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(),
                   ESPhttpUpdate.getLastErrorString().c_str());
            break;

        case HTTP_UPDATE_NO_UPDATES:
            DEBUGLN("HTTP_UPDATE_NO_UPDATES");
            break;

        case HTTP_UPDATE_OK:
            DEBUGLN("HTTP_UPDATE_OK");
            break;
    }
}