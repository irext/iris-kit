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

#include <Esp.h>
#include <WString.h>

#include "defines.h"
#include "IRbabyGlobal.h"
#include "IRbabySerial.h"
#include "IRbabyHttp.h"

#include "IRBabyIRIS.h"

extern StaticJsonDocument<1024> http_request_doc;
extern StaticJsonDocument<1024> http_response_doc;

char iris_credential_token[CREDENTIAL_MAX] = { 0 };
char iris_server_address[URL_SHORT_MAX] = { 0 };


int getIRISKitVersion(char *buffer, int buffer_size) {
    if (NULL == buffer) {
        return -1;
    }
    memset(buffer, 0, buffer_size);
    snprintf(buffer, buffer_size - 1, "%s_r%d", FIRMWARE_VERSION, VERSION_CODE);
    return strlen(buffer);
}

int getDeviceID(char* buffer, int buffer_size) {
    if (NULL == buffer) {
        return -1;
    }
    String chipId = String(ESP.getChipId(), HEX);
    memset(buffer, 0, buffer_size);
    snprintf(buffer, buffer_size - 1, "%s", chipId.c_str());
    return strlen(buffer);
}

int fetchIrisCredential(String credential_token,
                        String& product_key,
                        String& device_name,
                        String& device_secret) {
    int ret = -1;
    int tsi = -1;
    bool protocol_prefix = false;
    String fetch_credential_url;
    String request_data = "";
    String response_data = "";

    http_error_t http_ret = HTTP_ERROR_GENERIC;

    String device_id("IRbaby_");
    if (NULL != strstr(iris_server_address, "http://")) {
        protocol_prefix = true;
    }
    if (protocol_prefix) {
        fetch_credential_url = String(iris_server_address);
    } else {
        fetch_credential_url = String("http://");
        fetch_credential_url.concat(iris_server_address);
    }
    fetch_credential_url.concat(String(FETCH_CREDENTIAL_SUFFIX));
    device_id.concat(String(ESP.getChipId(), HEX));

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
    http_request_doc.clear();
    http_request_doc["deviceID"] = device_id;
    http_request_doc["credentialToken"] = credential_token;
    serializeJson(http_request_doc, request_data);

    http_ret = httpPost(fetch_credential_url, request_data, response_data);

    if (HTTP_ERROR_SUCCESS == http_ret) {
        http_response_doc.clear();
        if (OK == deserializeJson(http_response_doc, response_data.c_str())) {
            String ds = "";
            int resultCode = http_response_doc["status"]["code"];
            if (0 == resultCode) {
                INFOLN("response valid, try getting entity");
                ds = (String) http_response_doc["entity"];
                device_secret = ds;
                INFOF("HTTP response deserialized, PK = %s, DN = %s, DS = %s\n",
                    product_key.c_str(), device_name.c_str(), device_secret.c_str());
                ret = 0;
            } else {
                INFOF("response invalid, code = %d\n", resultCode);
            }
        }
    }

    return ret;
}

void sendIrisKitConnect() {

}

void sendIrisKitHeartBeat() {

}