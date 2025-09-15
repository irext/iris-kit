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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <LittleFS.h>
#include <ESP8266HTTPClient.h>

#include "defines.h"

#include "global.h"
#include "serials.h"

#include "http_client.h"

#define HTTP_REQUEST_MAX_RETRY        (5)
#define HTTP_REQUEST_RETRY_INTERVAL   (200)

http_error_t httpPost(String url, String request_data, String& result) {
    http_error_t ret = HTTP_ERROR_GENERIC;
    int response_code = 0;
    HTTPClient http_client;
    http_client.begin(wifi_client, url);
    http_client.addHeader("Content-Type", "application/json");
    bool request_flag = false;

    for (int http_retry = 0; http_retry < HTTP_REQUEST_MAX_RETRY; http_retry++) {
        response_code = http_client.POST(request_data);
        if (HTTP_CODE_OK == response_code) {
            INFOF("HTTP response OK : %d\n", response_code);
            String payload = http_client.getString();
            result = payload;
            request_flag = true;
            break;
        } else {
            ERRORF("HTTP response ERROR : %d\n", response_code);
            delay(HTTP_REQUEST_RETRY_INTERVAL);
        }
    }
    if (request_flag) {
        ret = HTTP_ERROR_SUCCESS;
    }
    http_client.end();
    return ret;
}

http_error_t downLoadFile(String url, String file, String path) {
    http_error_t ret = HTTP_ERROR_GENERIC;
    HTTPClient http_client;
    int response_code = 0;
    String save_path = path + file;
    File cache = LittleFS.open(save_path, "w");
    bool download_flag = false;

    if (cache) {
        http_client.begin(wifi_client, url);
        for (int http_retry = 0; http_retry < HTTP_REQUEST_MAX_RETRY; http_retry++) {
            response_code = http_client.GET();
            if (HTTP_CODE_OK == response_code) {
                download_flag = true;
                break;
            } else {
                ERRORF("HTTP response error : %d\n", response_code);
                delay(HTTP_REQUEST_RETRY_INTERVAL);
            }
        }
        if (download_flag) {
            http_client.writeToStream(&cache);
            ret = HTTP_ERROR_SUCCESS;
            INFOF("Download %s successfully\n", file.c_str());
        } else {
            LittleFS.remove(save_path);
            ret = HTTP_ERROR_GENERIC;
            ERRORF("Download %s failed\n", file.c_str());
        }
    } else {
        ret = HTTP_ERROR_LOCAL_SPACE;
        ERRORF("There is not enough storage space for file\n");
    }
    cache.close();
    http_client.end();

    return ret;
}
