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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <LittleFS.h>
#include <ESP8266HTTPClient.h>

#include "defines.h"
#include "IRbabyGlobal.h"
#include "IRbabySerial.h"

#include "IRbabyHttp.h"

#define FETCH_CREDENTIAL_SUFFIX      "/irext-collect/credentials/fetch_credential" 
#define DOWNLOAD_PREFIX              "http://irext-debug.oss-cn-hangzhou.aliyuncs.com/irda_"
#define DOWNLOAD_SUFFIX              ".bin"


extern StaticJsonDocument<1024> http_request_doc;
extern StaticJsonDocument<1024> http_response_doc;


char iris_server_address[URL_SHORT_MAX] = { 0 };


int fetchIrisCredential(String credential_token,
                        String& product_key,
                        String& device_name,
                        String& device_secret) {
    int ret = -1;

    String device_sn("IRbaby_");
    String fetch_credential_url(iris_server_address);
    HTTPClient http_client;
    int tsi = 0;
    int response_code = 0;
    fetch_credential_url.concat(String(FETCH_CREDENTIAL_SUFFIX));
    device_sn.concat(String(ESP.getChipId(), HEX));

    INFOF("fetch credential URL = %s\n", fetch_credential_url.c_str());
    if (credential_token.isEmpty()) {
        ERRORLN("credential token is empty");
        return -1;
    }
    tsi = credential_token.indexOf(",");
    if (-1 == tsi) {
        ERRORLN("credential token format error");
        return -1;
    }
    product_key = credential_token.substring(0, tsi);
    device_name = credential_token.substring(tsi + 1);

    http_client.begin(wifi_client, fetch_credential_url);
    http_client.addHeader("Content-Type", "application/json");
    http_request_doc.clear();
    http_request_doc["endpointSN"] = device_sn;
    http_request_doc["credentialToken"] = credential_token;
    String request_data = "";
    serializeJson(http_request_doc, request_data);

    response_code = http_client.POST(request_data);
    if (200 == response_code) {
        INFOF("HTTP response code = %d\n", response_code);
        String payload = http_client.getString();
        INFOF("HTTP response payload = %s\n", payload.c_str());
        http_response_doc.clear();
        if (OK == deserializeJson(http_response_doc, payload.c_str())) {
            String ds = http_response_doc["entity"];
            device_secret = ds;
            INFOF("HTTP response deserialized, PK = %s, DN = %s, DS = %s\n",
                   product_key.c_str(), device_name.c_str(), device_secret.c_str());
            ret = 0;
        }
    }

    http_client.end();

    return ret;
}


void downLoadFile(String file, String path) {
    HTTPClient http_client;
    String download_url = DOWNLOAD_PREFIX + file + DOWNLOAD_SUFFIX;
    String save_path = path + file;
    File cache = LittleFS.open(save_path, "w");
    bool download_flag = false;
    if (cache) {
        http_client.begin(wifi_client, download_url);
        for (int i = 0; i < 5; i++) {
            if (http_client.GET() == HTTP_CODE_OK) {
                download_flag = true;
                break;
            }
            delay(200);
        }
        if (download_flag) {
            http_client.writeToStream(&cache);
            DEBUGF("Download %s success\n", file.c_str());
        } else {
            LittleFS.remove(save_path);
            ERRORF("Download %s failed\n", file.c_str());
        }
    } else
        ERRORLN("Don't have enough file zoom");
    cache.close();
    http_client.end();
}
