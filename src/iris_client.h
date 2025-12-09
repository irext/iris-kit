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

#ifndef IRIS_KIT_IR_BABY_H
#define IRIS_KIT_IR_BABY_H

#define CREDENTIAL_MAX               (40)
#define USER_NAME_MAX                (64)
#define PASSWORD_MAX                 (64)

typedef enum {
    IRIS_KIT_STATUS_IDLE = 0,
    IRIS_KIT_STATUS_READY_TO_STUDY = 1,
    IRIS_KIT_STATUS_STUDIED = 2,
    IRIS_KIT_STATUS_UPLOADED = 3,
    IRIS_KIT_STATUS_CANCEL_STUDY = 4,
    IRIS_KIT_STATUS_EMITTING = 5,
    IRIS_KIT_STATUS_TEST = 10,

    IRIS_KIT_STATUS_NOT_CONNECTED = 62,
    IRIS_KIT_STATUS_MAX = 63,
} status_t;

typedef struct {
    int console_id;
    String remote_index;
    int key_id;
    String key_name;
    status_t status;
} iris_kit_status_t;

// web http call URL list
#define GET_IRIS_KIT_ACCOUNT_SUFFIX  "/irext-collect/credentials/auth_iris_kit"
#define LOAD_ALIOT_ACCOUNT_SUFFIX    "/irext-collect/aliot/load_account"
#define DOWNLOAD_BIN_SUFFIX          "/irext-collect/download"
#define DOWNLOAD_PREFIX              "http://irext-debug.oss-cn-hangzhou.aliyuncs.com/irda_"
#define DOWNLOAD_SUFFIX              ".bin"
#define RECEIVED_SUFFIX              ".rcv"

// IR bin code storage
#define SAVE_PATH                    "/ir/"

// IRIS communication
#define EVENT_NAME_CONNECT           "__connect"
#define EVENT_HEART_BEAT_REQ         "__hb_request"
#define EVENT_NOTIFY_RESP            "__notify_response"
#define EVENT_INDICATION             "__indication"

#define NOTIFY_RESP_TEST             "test_ok"
#define NOTIFY_STUDY_PREPARED        "study_prepared"
#define NOTIFY_STUDY_CANCELLED       "study_cancelled"
#define NOTIFY_STUDY_COMPLETED       "study_completed"
#define NOTIFY_STUDY_ERROR           "study_error"

typedef int (*eventHandler)(String, String, String);
typedef struct {
    const char* event_name;
    eventHandler handler;
} event_handler_t;

int getIRISKitVersion(char *buffer, int buffer_size);

String getDeviceID();

int authIrisKit(String credential_token,
                       String password,
                       String& product_key,
                       String& device_name,
                       String& device_secret,
                       String& device_token,
                       int& app_id);

void sendIrisKitConnect();

void sendIrisKitHeartBeat();

void handleIrisKitMessage(const char* data, int length);

int processStatusChange(int status,
                        int console_id,
                        String remote_index,
                        int key_id,
                        String key_name);

void updateIrisKitStatus(status_t status,
                        int console_id,
                        String remote_index,
                        int key_id, String
                        key_name);

void resetIrisKitStatus();

#endif // IRIS_KIT_IR_BABY_H