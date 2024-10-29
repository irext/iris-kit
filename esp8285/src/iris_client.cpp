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

#include <Esp.h>
#include <WString.h>
#include <LittleFS.h>

#include "defines.h"

#include "global.h"
#include "serials.h"
#include "iot_hub.h"
#include "http_client.h"
#include "ir_drv_ctrl.h"

#include "iris_client.h"


extern StaticJsonDocument<1024> http_request_doc;
extern StaticJsonDocument<1024> http_response_doc;
extern StaticJsonDocument<1024> iris_msg_doc;
extern StaticJsonDocument<1024> iris_ind_doc;
extern StaticJsonDocument<2048> emit_code_doc;
extern StaticJsonDocument<1024> status_notify_doc;

extern String g_product_key;
extern String g_device_name;
extern String g_upstream_topic;
extern int g_app_id;
extern iris_kit_status_t g_iris_kit_status;

char iris_credential_token[CREDENTIAL_MAX] = { 0 };
char iris_server_address[URL_SHORT_MAX] = { 0 };
char iris_user_name[USER_NAME_MAX] = { 0 };
char iris_password[PASSWORD_MAX] = { 0 };

// private function declarations
static int processEvent(String event_name, String product_key, String device_name, String content);
static String buildConnect();
static String buildHeartBeat();
static void buildGeneralResponse(String notify_name);
static void buildGeneralIndication(String notify_name);
static String buildTestResponse();
static String buildRecvPreparedResponse();
static String buildRecvCompletedIndication(String ir_data);
static String buildRecvErrorIndication();
static String buildRecvCancelledResponse();
static int handleConnected(String product_key, String device_name, String content);
static int handleHartBeat(String product_key, String device_name, String content);
static int handleEmit(String product_key, String device_name, String content);
static int handleNotifyStatus(String product_key, String device_name, String content);

static int hb_count = 0;

// private variable definitions
event_handler_t event_handler_table[] = {
    {
        "__connected",
        handleConnected,
    },
    {
        "__hb_response",
        handleHartBeat,
    },
    {
        "__emit_code",
        handleEmit,
    },
    {
        "__notify_status",
        handleNotifyStatus,
    }
};

// public function definitions
int getIRISKitVersion(char *buffer, int buffer_size) {
    if (NULL == buffer) {
        return -1;
    }
    memset(buffer, 0, buffer_size);
    snprintf(buffer, buffer_size - 1, "%s_r%d", FIRMWARE_VERSION, VERSION_CODE);
    return strlen(buffer);
}

String getDeviceID() {
    String device_id("IRISKit_");
    device_id.concat(String(ESP.getChipId(), HEX));
    return device_id;
}

int authIrisKit(String credential_token,
                    String password,
                    String& product_key,
                    String& device_name,
                    String& device_secret,
                    String& device_token,
                    int& app_id) {
    int ret = -1;
    int tsi = -1;
    bool protocol_prefix = false;
    String fetch_credential_url;
    String request_data = "";
    String response_data = "";
    String device_name_temp = "";

    http_error_t http_ret = HTTP_ERROR_GENERIC;

    if (NULL != strstr(iris_server_address, "http://")) {
        protocol_prefix = true;
    }
    if (protocol_prefix) {
        fetch_credential_url = String(iris_server_address);
    } else {
        fetch_credential_url = String("http://");
        fetch_credential_url.concat(iris_server_address);
    }
    fetch_credential_url.concat(String(GET_IRIS_KIT_ACCOUNT_SUFFIX));

    INFOF("Fetch credential URL = %s\n", fetch_credential_url.c_str());
    if (credential_token.isEmpty()) {
        ERRORF("Credential token is empty\n");
        return -1;
    }
    tsi = credential_token.indexOf(",");
    if (-1 == tsi) {
        ERRORF("Credential token format error\n");
        return -1;
    }
    product_key = credential_token.substring(0, tsi);
    device_name_temp = credential_token.substring(tsi + 1);
    tsi = device_name_temp.indexOf(",");
    device_name = device_name_temp.substring(0, tsi);

    http_request_doc.clear();
    http_request_doc["deviceID"] = getDeviceID();
    http_request_doc["credentialToken"] = credential_token;
    http_request_doc["password"] = password;
    serializeJson(http_request_doc, request_data);

    http_ret = httpPost(fetch_credential_url, request_data, response_data);

    if (HTTP_ERROR_SUCCESS == http_ret) {
        http_response_doc.clear();
        if (DeserializationError::Ok == deserializeJson(http_response_doc, response_data)) {
            int resultCode = http_response_doc["status"]["code"];
            if (0 == resultCode) {
                INFOF("response valid, try getting entity\n");
                // for aliot connection, use deviceToken as deviceSecret
                device_token = (const char*) http_response_doc["entity"]["deviceToken"];
                app_id = (int) http_response_doc["entity"]["appId"];
                INFOF("HTTP response deserialized, PK = %s, DN = %s, DS = %s, DT = %s\n",
                    product_key.c_str(), device_name.c_str(), device_secret.c_str(), device_token.c_str());
                ret = 0;
            } else {
                INFOF("Response invalid, code = %d\n", resultCode);
            }
        }
    }

    return ret;
}

bool downloadBin(int remote_id) {
    bool ret = false;
    bool protocol_prefix = false;
    String save_file = "";
    String download_bin_url;
    http_error_t http_ret = HTTP_ERROR_GENERIC;

    if (NULL != strstr(iris_server_address, "http://")) {
        protocol_prefix = true;
    }
    if (protocol_prefix) {
        download_bin_url = String(iris_server_address);
    } else {
        download_bin_url = String("http://");
        download_bin_url.concat(iris_server_address);
    }
    download_bin_url.concat(String(DOWNLOAD_BIN_SUFFIX));

    if (-1 != remote_id) {
        INFOF("Notified to download bin: %d\n", remote_id);
        save_file = String("ir_") + String(remote_id);

        String temp = String(SAVE_PATH) + String("/") + save_file;
        if (!LittleFS.exists(temp)) {
            downLoadFile(download_bin_url, save_file, SAVE_PATH);
            ret = true;
        }
    }
    return ret;
}

void sendIrisKitConnect() {
    String connectMessage = buildConnect();
    sendData(g_upstream_topic.c_str(), (uint8_t*) connectMessage.c_str(), connectMessage.length());
}

void sendIrisKitHeartBeat() {
    INFOF("Send iris kit heart beat[%d]\n", hb_count++);
    String heartBeatMessage = buildHeartBeat();
    sendData(g_upstream_topic.c_str(), (uint8_t*) heartBeatMessage.c_str(), heartBeatMessage.length());
}

void handleIrisKitMessage(const char* data, int length) {
    int ret = 0;
    char* payload = (char*) malloc(length + 1);
    if (NULL != payload) {
        strncpy(payload, data, length);
        payload[length] = '\0';
        INFOF("--> %s\n", payload);
        if (DeserializationError::Ok == deserializeJson(iris_ind_doc, payload)) {
            String event_name = iris_ind_doc["eventName"];
            String product_key = iris_ind_doc["productKey"];
            String device_name = iris_ind_doc["deviceName"];
            String content = iris_ind_doc["content"];
            INFOF("Received ind : %s\n", event_name.c_str());
            ret = processEvent(event_name.c_str(), product_key, device_name, content);
            INFOF("Event handle result = %d\n", ret);
        }
    }

    if (NULL != payload) {
        free(payload);
    }
}

int processStatusChange(int status, int console_id, int key_id, String key_name, String remote_index) {
    switch(status) {
        case IRIS_KIT_STATUS_READY_TO_STUDY:
        {
            // enter into IR receive mode and send response
            updateIrisKitStatus(IRIS_KIT_STATUS_READY_TO_STUDY, console_id, remote_index, key_id, key_name);
            prepareRecvIR();
            String recvPreparedResponseData = buildRecvPreparedResponse();
            sendData(g_upstream_topic.c_str(), (uint8_t*) recvPreparedResponseData.c_str(), recvPreparedResponseData.length());
            break;
        }
        case IRIS_KIT_STATUS_STUDIED:
        {
            // after IR data are received, load saved IR data and send to IRIS server
            updateIrisKitStatus(IRIS_KIT_STATUS_STUDIED, console_id, remote_index, key_id, key_name);
            String ir_data = "";
            String recvCompletedIndicationData = "";
            if (completeRecvIR(ir_data) > 0) {
                recvCompletedIndicationData = buildRecvCompletedIndication(ir_data);
            } else {
                recvCompletedIndicationData = buildRecvErrorIndication();
            }
            sendData(g_upstream_topic.c_str(), (uint8_t*) recvCompletedIndicationData.c_str(), recvCompletedIndicationData.length());
            updateIrisKitStatus(IRIS_KIT_STATUS_UPLOADED, console_id, remote_index, key_id, key_name);
            break;
        }
        case IRIS_KIT_STATUS_CANCEL_STUDY:
        {
            // cancel IR receiving and reset
            cancelRecvIR();
            String studyCancelledResponseData = buildRecvCancelledResponse();
            sendData(g_upstream_topic.c_str(), (uint8_t*) studyCancelledResponseData.c_str(), studyCancelledResponseData.length());
            resetIrisKitStatus();
            break;
        }
        case IRIS_KIT_STATUS_TEST:
        {
            // send response for test notification
            updateIrisKitStatus(IRIS_KIT_STATUS_TEST, console_id, remote_index, key_id, key_name);
            String testResponseData = buildTestResponse();
            sendData(g_upstream_topic.c_str(), (uint8_t*) testResponseData.c_str(), testResponseData.length());
            resetIrisKitStatus();
            break;
        }
        default:
        {
            break;
        }
    }

    return 0;
}

void updateIrisKitStatus(status_t status,
                        int console_id,
                        String remote_index,
                        int key_id, String
                        key_name) {
    g_iris_kit_status.status = status;
    g_iris_kit_status.console_id = console_id;
    g_iris_kit_status.remote_index = remote_index;
    g_iris_kit_status.key_id = key_id;
    g_iris_kit_status.key_name = key_name;
}

void resetIrisKitStatus() {
    g_iris_kit_status.status = IRIS_KIT_STATUS_IDLE;
    g_iris_kit_status.console_id = 0;
    g_iris_kit_status.remote_index = "";
    g_iris_kit_status.key_id = 0;
    g_iris_kit_status.key_name = "";
}


// private function definitions
static int processEvent(String event_name, String product_key, String device_name, String content) {
    int event_table_length = sizeof(event_handler_table) / sizeof(event_handler_table[0]);
    for (int i = 0; i < event_table_length; i++) {
        if (0 == strcmp(event_name.c_str(), event_handler_table[i].event_name)) {
            INFOF("Call event handler with payload : %s, %s\n", product_key.c_str(), device_name.c_str());
            return event_handler_table[i].handler(product_key, device_name, content);
        }
    }
    return -1;
}

static String buildConnect() {
    String connectMessage = "";

    iris_msg_doc.clear();
    iris_msg_doc["eventName"] = String(EVENT_NAME_CONNECT);
    iris_msg_doc["productKey"] = g_product_key;
    iris_msg_doc["deviceName"] = g_device_name;
    serializeJson(iris_msg_doc, connectMessage);

    return connectMessage;
}

static String buildHeartBeat() {
    String heartBeatMessage = "";

    iris_msg_doc.clear();
    iris_msg_doc["eventName"] = String(EVENT_HEART_BEAT_REQ);
    iris_msg_doc["productKey"] = g_product_key;
    iris_msg_doc["deviceName"] = g_device_name;
    iris_msg_doc["appId"] = g_app_id;
    serializeJson(iris_msg_doc, heartBeatMessage);

    return heartBeatMessage;
}

static void buildGeneralResponse(String notify_name) {
    iris_msg_doc.clear();
    iris_msg_doc["eventName"] = String(EVENT_NOTIFY_RESP);
    iris_msg_doc["productKey"] = g_product_key;
    iris_msg_doc["deviceName"] = g_device_name;
    iris_msg_doc["appId"] = g_app_id;
    iris_msg_doc["consoleId"] = g_iris_kit_status.console_id;
    iris_msg_doc["resp"] = String(notify_name);
}

static void buildGeneralIndication(String notify_name) {
    iris_ind_doc.clear();
    iris_ind_doc["eventName"] = String(EVENT_NOTIFY_RESP);
    iris_ind_doc["productKey"] = g_product_key;
    iris_ind_doc["deviceName"] = g_device_name;
    iris_ind_doc["appId"] = g_app_id;
    iris_ind_doc["consoleId"] = g_iris_kit_status.console_id;
    iris_ind_doc["remoteIndex"] = g_iris_kit_status.remote_index;
    iris_ind_doc["keyId"] = g_iris_kit_status.key_id;
    iris_ind_doc["keyName"] = g_iris_kit_status.key_name;
    iris_ind_doc["resp"] = String(notify_name);
}

static String buildTestResponse() {
    String testReponse = "";
    buildGeneralResponse(NOTIFY_RESP_TEST);
    serializeJson(iris_msg_doc, testReponse);

    return testReponse;
}

static String buildRecvPreparedResponse() {
    String recvPreparedResponse = "";
    buildGeneralResponse(NOTIFY_RECV_PREPARED);
    serializeJson(iris_msg_doc, recvPreparedResponse);

    return recvPreparedResponse;
}

static String buildRecvCompletedIndication(String ir_data) {
    String recvCompletedIndication = "";
    buildGeneralIndication(NOTIFY_RECV_COMPLETED);
    iris_ind_doc["payload"] = ir_data;
    serializeJson(iris_ind_doc, recvCompletedIndication);

    return recvCompletedIndication;
}

static String buildRecvErrorIndication() {
    String recvErrorIndication = "";
    buildGeneralIndication(NOTIFY_RECV_COMPLETED);
    iris_ind_doc["payload"] = "error";
    serializeJson(iris_ind_doc, recvErrorIndication);

    return recvErrorIndication;
}

static String buildRecvCancelledResponse() {
    String recvCancelledResponse = "";
    buildGeneralResponse(NOTIFY_RECV_CANCELLED);
    serializeJson(iris_msg_doc, recvCancelledResponse);

    return recvCancelledResponse;
}

static int handleConnected(String product_key, String device_name, String content) {
    return 0;
}

static int handleHartBeat(String product_key, String device_name, String content) {
    INFOF("Received heartbeat : %s, %s\n", product_key.c_str(), device_name.c_str());
    return 0;
}

static int handleEmit(String product_key, String device_name, String content) {
    INFOF("Received emit code : %s, %s, %s\n", product_key.c_str(), device_name.c_str(), content.c_str());
    updateIrisKitStatus(IRIS_KIT_STATUS_EMITTING, 0, "", 0, "");
    emit_code_doc.clear();
    if (DeserializationError::Ok == deserializeJson(emit_code_doc, content)) {
        int remote_id = emit_code_doc["remoteId"];
        String key_name = emit_code_doc["keyName"];
        String key_value = emit_code_doc["keyValue"];
        INFOF("Emitting key : %s for remote %d = %s\n", key_name.c_str(), remote_id, key_value.c_str());
        emitIR(key_value);
    } else {
        INFOF("Deserialize failed\n");
    }
    resetIrisKitStatus();
    return 0;
}

static int handleNotifyStatus(String product_key, String device_name, String content) {
    INFOF("Received status change notification : %s, %s, %s\n", product_key.c_str(), device_name.c_str(), content.c_str());
    status_notify_doc.clear();
    if (DeserializationError::Ok == deserializeJson(status_notify_doc, content)) {
        String remote_index = status_notify_doc["remoteIndex"];
        int key_id = status_notify_doc["keyId"];
        String key_name = status_notify_doc["keyName"];
        int status = status_notify_doc["status"];
        int console_id = status_notify_doc["consoleId"];
        INFOF("Entering status : %d for %s(%d)\n", status, key_name.c_str(), key_id);

        processStatusChange(status, console_id, key_id, key_name, remote_index);
    } else {
        INFOF("Deserialize failed\n");
    }
    return 0;
}