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

#ifndef IRIS_KIT_IR_BABY_H
#define IRIS_KIT_IR_BABY_H

#define CREDENTIAL_MAX               (40)
#define USER_NAME_MAX                (64)
#define PASSWORD_MAX                 (64)

typedef enum {
    RECIPIENT_STATUS_IDLE = 0,
    RECIPIENT_STATUS_READY_TO_STUDY = 1,
    RECIPIENT_STATUS_STUDIED = 2,
    RECIPIENT_STATUS_UPLOADED = 3,
    RECIPIENT_STATUS_CANCEL_STUDY = 4,
    RECIPIENT_STATUS_TEST = 10,
    RECIPIENT_STATUS_MAX = 63,
} kit_status_t;

// web http call URL list
#define GET_IRIS_KIT_ACCOUNT_SUFFIX  "/irext-collect/credentials/auth_iris_kit"
#define LOAD_ALIOT_ACCOUNT_SUFFIX    "/irext-collect/aliot/load_account"
#define DOWNLOAD_BIN_SUFFIX          "/irext-collect/download"
#define DOWNLOAD_PREFIX              "http://irext-debug.oss-cn-hangzhou.aliyuncs.com/irda_"
#define DOWNLOAD_SUFFIX              ".bin"

// IRext bin code storage
#define SAVE_PATH                    "/ir/"

// IRIS communication
#define EVENT_NAME_CONNECT           "__connect"
#define EVENT_HEART_BEAT_REQ         "__hb_request"
#define EVENT_NOTIFY_RESP            "__notify_response"

#define NOTIFY_RESP_TEST             "test_ok"
#define NOTIFY_RECV_PREPARED         "recv_prepared"
#define NOTIFY_STUDY_CANCELLED       "study_cancelled"

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

#endif // IRIS_KIT_IR_BABY_H