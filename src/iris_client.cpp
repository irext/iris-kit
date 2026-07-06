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
#include "ota_manager.h"

#include "iris_kit.h"

#include "iris_client.h"


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

static String buildTestResponse(int console_id);

static String buildStudyPreparedResponse(int console_id, String remote_index, int key_id, String key_name);

static String buildStudyCompletedIndication(String ir_data,
                                            int console_id, String remote_index, int key_id, String key_name);

static String buildStudyErrorIndication(int console_id, String remote_index, int key_id, String key_name);

static String buildStudyCancelledResponse(int console_id, String remote_index, int key_id, String key_name);

static int handleConnected(String product_key, String device_name, String content);

static int handleHeartBeat(String product_key, String device_name, String content);

static int handleEmit(String product_key, String device_name, String content);

static int handleNotifyStatus(String product_key, String device_name, String content);

static int handleReboot(String product_key, String device_name, String content);

static int handleReset(String product_key, String device_name, String content);

static int compareVersions(const String& versionA, const String& versionB);

static int handleFirmwareUpdate(String product_key, String device_name, String content);

static int hb_count = 0;


// private variable definitions
event_handler_t event_handler_table[] = {
    {
        .event_name = EVENT_NAME_CONNECTED,
        .handler = handleConnected,
    },
    {
        .event_name = EVENT_NAME_HB_RESPONSE,
        .handler = handleHeartBeat,
    },
    {
        .event_name = EVENT_NAME_EMIT_CODE,
        .handler = handleEmit,
    },
    {
        .event_name = EVENT_NAME_NOTIFY_STATUS,
        .handler = handleNotifyStatus,
    },
    {
        .event_name = EVENT_NAME_REBOOT,
        .handler = handleReboot,
    },
    {
        .event_name = EVENT_NAME_RESET,
        .handler = handleReset,
    },
    {
        .event_name = EVENT_NAME_FIRMWARE_UPDATE,
        .handler = handleFirmwareUpdate,
    }
};

// public function definitions
int getIRISKitVersion(char *buffer, int buffer_size) {
    if (nullptr == buffer) {
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

    if (nullptr != strstr(iris_server_address, "http://")) {
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

    if (nullptr != strstr(iris_server_address, "http://")) {
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
    if (nullptr != payload) {
        strncpy(payload, data, length);
        payload[length] = '\0';
        INFOF("--> %s\n", payload);
        if (DeserializationError::Ok == deserializeJson(mqtt_downstream_topic_msg_doc, payload)) {
            String event_name = mqtt_downstream_topic_msg_doc["eventName"];
            String product_key = mqtt_downstream_topic_msg_doc["productKey"];
            String device_name = mqtt_downstream_topic_msg_doc["deviceName"];
            String content = mqtt_downstream_topic_msg_doc["content"];
            INFOF("Received message : %s\n", event_name.c_str());
            ret = processEvent(event_name.c_str(), product_key, device_name, content);
            INFOF("Event handle result = %d\n", ret);
        }
    }

    if (nullptr != payload) {
        free(payload);
    }
}

int processStatusChange(int status, int console_id, String remote_index, int key_id, String key_name) {
    switch(status) {
        case IRIS_KIT_STATUS_READY_TO_STUDY:
        {
            // enter into IR receive mode and send response
            updateIrisKitStatus(IRIS_KIT_STATUS_READY_TO_STUDY, console_id, remote_index, key_id, key_name);
            prepareStudyIR();
            String recvPreparedResponseData = buildStudyPreparedResponse(console_id, remote_index, key_id, key_name);
            sendData(g_upstream_topic.c_str(), (uint8_t*) recvPreparedResponseData.c_str(), recvPreparedResponseData.length());
            break;
        }
        case IRIS_KIT_STATUS_STUDIED:
        {
            // after IR data are received, load saved IR data and send to IRIS server
            updateIrisKitStatus(IRIS_KIT_STATUS_STUDIED, console_id, remote_index, key_id, key_name);
            String ir_data = "";
            String studyCompletedIndicationData = "";
            if (completeStudyIR(ir_data) > 0) {
                studyCompletedIndicationData = buildStudyCompletedIndication(ir_data, console_id, remote_index, key_id, key_name);
            } else {
                studyCompletedIndicationData = buildStudyErrorIndication(console_id, remote_index, key_id, key_name);
            }
            sendData(g_upstream_topic.c_str(), (uint8_t*) studyCompletedIndicationData.c_str(), studyCompletedIndicationData.length());
            updateIrisKitStatus(IRIS_KIT_STATUS_UPLOADED, console_id, remote_index, key_id, key_name);
            break;
        }
        case IRIS_KIT_STATUS_CANCEL_STUDY:
        {
            // cancel IR receiving and reset
            cancelStudyIR();
            String studyCancelledResponseData = buildStudyCancelledResponse(console_id, remote_index, key_id, key_name);
            sendData(g_upstream_topic.c_str(), (uint8_t*) studyCancelledResponseData.c_str(), studyCancelledResponseData.length());
            resetIrisKitStatus();
            break;
        }
        case IRIS_KIT_STATUS_TEST:
        {
            // send response for test notification
            updateIrisKitStatus(IRIS_KIT_STATUS_TEST, console_id, remote_index, key_id, key_name);
            String testResponseData = buildTestResponse(console_id);
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
                        int key_id,
                        String key_name) {
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
static String buildConnect() {
    String connectMessage = "";
    char version_buffer[32] = { 0 };

    mqtt_upstream_topic_msg_doc.clear();
    mqtt_upstream_topic_msg_doc["eventName"] = String(EVENT_NAME_CONNECT);
    mqtt_upstream_topic_msg_doc["productKey"] = g_product_key;
    mqtt_upstream_topic_msg_doc["deviceName"] = g_device_name;
    getIRISKitVersion(version_buffer, sizeof(version_buffer));
    mqtt_upstream_topic_msg_doc["currentVersion"] = String(version_buffer);
    serializeJson(mqtt_upstream_topic_msg_doc, connectMessage);

    return connectMessage;
}

static String buildHeartBeat() {
    String heartBeatMessage = "";
    char version_buffer[32] = { 0 };

    mqtt_upstream_topic_hbt_doc.clear();
    mqtt_upstream_topic_hbt_doc["eventName"] = String(EVENT_HEART_BEAT_REQ);
    mqtt_upstream_topic_hbt_doc["productKey"] = g_product_key;
    mqtt_upstream_topic_hbt_doc["deviceName"] = g_device_name;
    mqtt_upstream_topic_hbt_doc["appId"] = g_app_id;
    getIRISKitVersion(version_buffer, sizeof(version_buffer));
    mqtt_upstream_topic_hbt_doc["currentVersion"] = String(version_buffer);
    
    serializeJson(mqtt_upstream_topic_hbt_doc, heartBeatMessage);

    return heartBeatMessage;
}

static void buildGeneralResponse(String notify_name, StaticJsonDocument<512> &json_doc,
                                 int console_id, String remote_index, int key_id, String key_name) {
    json_doc.clear();
    json_doc["productKey"] = g_product_key;
    json_doc["deviceName"] = g_device_name;
    json_doc["appId"] = g_app_id;
    json_doc["consoleId"] = console_id;
    json_doc["remoteIndex"] = remote_index;
    json_doc["keyId"] = key_id;
    json_doc["keyName"] = key_name;
    json_doc["resp"] = String(notify_name);
    json_doc["eventName"] = String(EVENT_NOTIFY_RESP);
}

static void buildGeneralIndication(String notify_name, StaticJsonDocument<2048> &json_doc,
                                   int console_id, String remote_index, int key_id, String key_name) {
    json_doc.clear();
    json_doc["productKey"] = g_product_key;
    json_doc["deviceName"] = g_device_name;
    json_doc["appId"] = g_app_id;
    json_doc["consoleId"] = console_id;
    json_doc["remoteIndex"] = remote_index;
    json_doc["keyId"] = key_id;
    json_doc["keyName"] = key_name;
    json_doc["resp"] = String(notify_name);
    json_doc["eventName"] = String(EVENT_INDICATION);
}

static String buildTestResponse(int console_id) {
    String testReponse = "";
    buildGeneralResponse(NOTIFY_RESP_TEST, mqtt_upstream_topic_rsp_doc, console_id, "", 0, "");
    serializeJson(mqtt_upstream_topic_rsp_doc, testReponse);

    return testReponse;
}

static String buildStudyPreparedResponse(int console_id, String remote_index, int key_id, String key_name) {
    String studyPreparedResponse = "";
    buildGeneralResponse(NOTIFY_STUDY_PREPARED, mqtt_upstream_topic_rsp_doc,
                         console_id, remote_index, key_id, key_name);
    serializeJson(mqtt_upstream_topic_rsp_doc, studyPreparedResponse);

    return studyPreparedResponse;
}

static String buildStudyCompletedIndication(String ir_data,
                                            int console_id, String remote_index, int key_id, String key_name) {
    String studyCompletedIndication = "";
    buildGeneralIndication(NOTIFY_STUDY_COMPLETED, mqtt_upstream_topic_ind_doc,
                           console_id, remote_index, key_id, key_name);
    mqtt_upstream_topic_ind_doc["payload"] = ir_data;
    serializeJson(mqtt_upstream_topic_ind_doc, studyCompletedIndication);

    return studyCompletedIndication;
}

static String buildStudyErrorIndication(int console_id, String remote_index, int key_id, String key_name) {
    String studyErrorIndication = "";
    buildGeneralIndication(NOTIFY_STUDY_ERROR, mqtt_upstream_topic_ind_doc,
                           console_id, remote_index, key_id, key_name);
    serializeJson(mqtt_upstream_topic_ind_doc, studyErrorIndication);

    return studyErrorIndication;
}

static String buildStudyCancelledResponse(int console_id, String remote_index, int key_id, String key_name) {
    String studyCancelledResponse = "";
    buildGeneralResponse(NOTIFY_STUDY_CANCELLED, mqtt_upstream_topic_rsp_doc,
                         console_id, remote_index, key_id, key_name);
    serializeJson(mqtt_upstream_topic_rsp_doc, studyCancelledResponse);

    return studyCancelledResponse;
}

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

static int handleConnected(String product_key, String device_name, String content) {
    return 0;
}

static int handleHeartBeat(String product_key, String device_name, String content) {
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

        processStatusChange(status, console_id, remote_index, key_id, key_name);
    } else {
        INFOF("Deserialize failed\n");
    }
    return 0;
}

static int handleReboot(String product_key, String device_name, String content) {
    (void) content;
    
    // Ignore reboot command during OTA upgrade
    ota_status_t ota_status = getOTAStatus();
    if (ota_status == OTA_STATUS_DOWNLOADING || ota_status == OTA_STATUS_REBOOTING) {
        INFOF("Ignoring reboot command during OTA upgrade (status=%d)\n", ota_status);
        return -1;
    }
    
    if (product_key.isEmpty() || device_name.isEmpty()) {
        ERROR("Invalid product key or device name\n");
        return -1;
    }
    INFOF("Received reboot command, product_key = %s, device_name = %s\n", product_key.c_str(), device_name.c_str());
    if (product_key.equals(g_product_key) && device_name.equals(g_device_name)) {
        wifiRestart();
    }
    return 0;
}

static int handleReset(String product_key, String device_name, String content) {
    (void) content;
    
    // Ignore reset command during OTA upgrade
    ota_status_t ota_status = getOTAStatus();
    if (ota_status == OTA_STATUS_DOWNLOADING || ota_status == OTA_STATUS_REBOOTING) {
        INFOF("Ignoring reset command during OTA upgrade (status=%d)\n", ota_status);
        return -1;
    }
    
    if (product_key.isEmpty() || device_name.isEmpty()) {
        ERROR("Invalid product key or device name\n");
        return -1;
    }
    INFOF("Received reset command, product_key = %s, device_name = %s\n", product_key.c_str(), device_name.c_str());
    if (product_key.equals(g_product_key) && device_name.equals(g_device_name)) {
        wifiReset();
    }
    return 0;
}

static int handleFirmwareUpdate(String product_key, String device_name, String content) {
    // Ignore firmware update command if already upgrading
    ota_status_t ota_status = getOTAStatus();
    if (ota_status == OTA_STATUS_DOWNLOADING || ota_status == OTA_STATUS_REBOOTING) {
        INFOF("Ignoring firmware update command during OTA upgrade (status=%d)\n", ota_status);
        return -1;
    }
    
    if (product_key.isEmpty() || device_name.isEmpty()) {
        ERROR("Invalid product key or device name\n");
        return -1;
    }

    if (!product_key.equals(g_product_key) || !device_name.equals(g_device_name)) {
        ERROR("Product key or device name mismatch\n");
        return -1;
    }

    INFOF("Received firmware_update command, content = %s\n", content.c_str());

    StaticJsonDocument<512> doc;
    if (DeserializationError::Ok != deserializeJson(doc, content)) {
        ERROR("Failed to parse firmware update content\n");
        return -1;
    }

    String firmware_url = doc["downloadUrl"];
    String target_version = doc["version"];
    String name = doc["name"];

    if (firmware_url.isEmpty()) {
        ERROR("Download URL is empty\n");
        return -1;
    }

    if (!name.equals("iris_kit")) {
        ERRORF("Component name mismatch: expected 'iris_kit', got '%s'\n", name.c_str());

        mqtt_upstream_topic_msg_doc.clear();
        mqtt_upstream_topic_msg_doc["eventName"] = "__firmware_update_progress";
        mqtt_upstream_topic_msg_doc["productKey"] = g_product_key;
        mqtt_upstream_topic_msg_doc["deviceName"] = g_device_name;
        mqtt_upstream_topic_msg_doc["appId"] = g_app_id;
        mqtt_upstream_topic_msg_doc["consoleId"] = 0;
        mqtt_upstream_topic_msg_doc["remoteIndex"] = "";
        mqtt_upstream_topic_msg_doc["keyId"] = 0;
        mqtt_upstream_topic_msg_doc["keyName"] = "";
        mqtt_upstream_topic_msg_doc["resp"] = "ota_progress";

        StaticJsonDocument<256> payload_doc;
        payload_doc["stage"] = "failed";
        payload_doc["progress"] = 0;
        payload_doc["message"] = "Component name mismatch";
        payload_doc["success"] = false;

        String payload_str;
        serializeJson(payload_doc, payload_str);
        mqtt_upstream_topic_msg_doc["payload"] = payload_str;

        String mqtt_message;
        serializeJson(mqtt_upstream_topic_msg_doc, mqtt_message);
        sendData(g_upstream_topic.c_str(), (uint8_t*)mqtt_message.c_str(), mqtt_message.length());

        return -1;
    }

    char current_version_buf[32] = { 0 };
    getIRISKitVersion(current_version_buf, sizeof(current_version_buf));
    String current_version = String(current_version_buf);

    int version_compare = compareVersions(target_version, current_version);
    if (version_compare <= 0) {
        INFOF("Target version %s is not greater than current version %s, skip upgrade\n",
              target_version.c_str(), current_version.c_str());

        mqtt_upstream_topic_msg_doc.clear();
        mqtt_upstream_topic_msg_doc["eventName"] = "__firmware_update_progress";
        mqtt_upstream_topic_msg_doc["productKey"] = g_product_key;
        mqtt_upstream_topic_msg_doc["deviceName"] = g_device_name;
        mqtt_upstream_topic_msg_doc["appId"] = g_app_id;
        mqtt_upstream_topic_msg_doc["consoleId"] = 0;
        mqtt_upstream_topic_msg_doc["remoteIndex"] = "";
        mqtt_upstream_topic_msg_doc["keyId"] = 0;
        mqtt_upstream_topic_msg_doc["keyName"] = "";
        mqtt_upstream_topic_msg_doc["resp"] = "ota_progress";

        StaticJsonDocument<256> payload_doc;
        payload_doc["stage"] = "failed";
        payload_doc["progress"] = 0;
        payload_doc["message"] = "Target version is not greater than current version";
        payload_doc["success"] = false;

        String payload_str;
        serializeJson(payload_doc, payload_str);
        mqtt_upstream_topic_msg_doc["payload"] = payload_str;

        String mqtt_message;
        serializeJson(mqtt_upstream_topic_msg_doc, mqtt_message);
        sendData(g_upstream_topic.c_str(), (uint8_t*)mqtt_message.c_str(), mqtt_message.length());

        return -1;
    }

    INFOF("Starting OTA upgrade: version=%s, name=%s, url=%s\n",
          target_version.c_str(), name.c_str(), firmware_url.c_str());

    return startOTAUpgrade(firmware_url, target_version);
}

static int compareVersions(const String& versionA, const String& versionB) {
    if (versionA.isEmpty() || versionB.isEmpty()) {
        return 0;
    }

    int sepA = versionA.indexOf("_r");
    int sepB = versionB.indexOf("_r");

    if (sepA == -1 || sepB == -1) {
        return 0;
    }

    String verA = versionA.substring(0, sepA);
    String verB = versionB.substring(0, sepB);

    int dotA1 = verA.indexOf('.');
    int dotA2 = verA.indexOf('.', dotA1 + 1);
    int dotB1 = verB.indexOf('.');
    int dotB2 = verB.indexOf('.', dotB1 + 1);

    if (dotA1 == -1 || dotA2 == -1 || dotB1 == -1 || dotB2 == -1) {
        return 0;
    }

    int majorA = verA.substring(0, dotA1).toInt();
    int minorA = verA.substring(dotA1 + 1, dotA2).toInt();
    int patchA = verA.substring(dotA2 + 1).toInt();

    int majorB = verB.substring(0, dotB1).toInt();
    int minorB = verB.substring(dotB1 + 1, dotB2).toInt();
    int patchB = verB.substring(dotB2 + 1).toInt();

    if (majorA != majorB) {
        return majorA > majorB ? 1 : -1;
    }
    if (minorA != minorB) {
        return minorA > minorB ? 1 : -1;
    }
    if (patchA != patchB) {
        return patchA > patchB ? 1 : -1;
    }

    String relA = versionA.substring(sepA + 2);
    String relB = versionB.substring(sepB + 2);

    int releaseA = relA.toInt();
    int releaseB = relB.toInt();

    if (releaseA != releaseB) {
        return releaseA > releaseB ? 1 : -1;
    }

    return 0;
}
