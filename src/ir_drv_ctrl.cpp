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

#include <LittleFS.h>

#include "global.h"
#include "serials.h"
#include "user_settings.h"
#include "iris_client.h"
#include "http_client.h"
#include "utils.h"

#include "ir_drv_ctrl.h"

#define IR_SERIES_MAX    (1024)
#define IR_END_CODE      (10000)


// external variable declarations
extern iris_kit_status_t g_iris_kit_status;


// public variable definitions
decode_results g_recv_results;

String g_recv_ir_code_str = "";


// private variable definitions
static IRsend * ir_send = nullptr;
static IRrecv * ir_recv = nullptr;
static const uint8_t k_timeout = 50;
// as this program is a special purpose capture/decoder, let us use a larger
// than normal buffer so we can handle Air Conditioner remote codes
static const uint16_t k_capture_buffer_size = IR_SERIES_MAX;


// public function definitions
bool sendIR(String file_name) {
    String save_path = SAVE_PATH + file_name;
    if (LittleFS.exists(save_path)) {
        File cache = LittleFS.open(save_path, "r");
        if (!cache) {
            ERRORF("Failed to open %s\n", save_path.c_str());
            return false;
        }
        Serial.println();
        uint16_t *data_buffer = (uint16_t *)malloc(sizeof(uint16_t) * IR_SERIES_MAX);
        uint16_t length = cache.size() / 2;
        memset(data_buffer, 0x0, IR_SERIES_MAX);
        cache.readBytes((char *)data_buffer, cache.size());
        ir_recv->disableIRIn();
        ir_send->sendRaw(data_buffer, length, 38);
        free(data_buffer);
        cache.close();
        return true;
    }
    return false;
}

bool emitIR(String timing) {
    char* parts[IR_SERIES_MAX];
    uint16_t series[IR_SERIES_MAX + 1] = { 0 };
    int parts_num = 0;
    int i = 0;

    parts_num = split_string(timing, parts, ",");
    if (parts_num > 0) {
        for (i = 0; i < parts_num; i++) {
            series[i] = (uint16_t) atoi(parts[i]);
        }
        series[i] = (uint16_t) IR_END_CODE;
        ir_recv->disableIRIn();
        INFOF("IR send raw : %d\n", parts_num);
        ir_send->sendRaw(series, parts_num + 1, 38);
    }
    return true;
}

void prepareStudyIR() {
#if defined STORE_RECEIVED_IR_DATA
    removeReceived();
#endif
    enableIRIn();
}

void cancelStudyIR() {
    disableIRIn();
}

int completeStudyIR(String &ir_data) {
    // called unsolicited
    disableIRIn();
#if defined STORE_RECEIVED_IR_DATA
    loadReceived(ir_data);
#else
    ir_data = g_recv_ir_code_str;
#endif
    DEBUGF("loaded received code : %s\n", ir_data.c_str());

    return ir_data.length();
}

void recvIR() {
    int max_time_slice = 0;
    int time_slice = 0;
    if (ir_recv->decode(&g_recv_results)) {
        INFOF("Recv IR, raw length = %d\n", g_recv_results.rawlen - 1);
        g_recv_ir_code_str.clear();
        for (int i = 1; i < g_recv_results.rawlen; i++) {
            time_slice = *(g_recv_results.rawbuf + i) * kRawTick;
            if (time_slice > max_time_slice) {
                max_time_slice = time_slice;
            }
            g_recv_ir_code_str += String(time_slice) + ",";
        }
        g_recv_ir_code_str += String(3 * max_time_slice);

        ir_recv->resume();

        DEBUGLN(g_recv_ir_code_str.c_str());

#if defined STORE_RECEIVED_IR_DATA
        saveReceived(g_recv_ir_code_str);
#endif

        processStatusChange(IRIS_KIT_STATUS_STUDIED,
                            g_iris_kit_status.console_id,
                            g_iris_kit_status.remote_index,
                            g_iris_kit_status.key_id,
                            g_iris_kit_status.key_name);
    }
}

bool saveReceived(String& ir_data) {
    String save_path = SAVE_PATH;
    String file_name = "";

    if (g_iris_kit_status.remote_index.isEmpty()) {
        return false;
    }

    file_name = "ir_" + g_iris_kit_status.remote_index + RECEIVED_SUFFIX;
    save_path += file_name;
    INFOF("Save received code to: %s\n", save_path.c_str());
    File cache = LittleFS.open(save_path, "w");
    if (!cache) {
        ERRORF("Failed to create file\n");
        return false;
    }

    cache.write(ir_data.c_str(), ir_data.length());
    cache.close();

    return true;
}

bool removeReceived() {
    String save_path = SAVE_PATH;
    String file_name = "";

    if (g_iris_kit_status.remote_index.isEmpty()) {
        return false;
    }

    file_name = "ir_" + g_iris_kit_status.remote_index + RECEIVED_SUFFIX;
    save_path += file_name;
    INFOF("Delete received code file: %s\n", save_path.c_str());
    LittleFS.remove(save_path);

    return true;
}

int loadReceived(String &ir_data) {
    String save_path = SAVE_PATH;
    String file_name = "";

    if (g_iris_kit_status.remote_index.isEmpty()) {
        return -1;
    }

    file_name = "ir_" + g_iris_kit_status.remote_index + RECEIVED_SUFFIX;
    save_path += file_name;
    DEBUGF("Load received code from: %s\n", save_path.c_str());

    File cache = LittleFS.open(save_path, "r");
    if (!cache) {
        ERRORF("Failed to open file\n");
        return false;
    }
    ir_data = cache.readString();
    cache.close();

    DEBUGF("received code : %s\n", ir_data.c_str());

    return ir_data.length();
}

void initAC(String file) {
    ACStatus[file]["power"] = 0;
    ACStatus[file]["temperature"] = 8;
    ACStatus[file]["mode"] = 2;
    ACStatus[file]["swing"] = 0;
    ACStatus[file]["speed"] = 0;
}

void loadIRPin(uint8_t send_pin, uint8_t recv_pin) {
    if (ir_send != nullptr) {
        delete ir_send;
    }
    if (ir_recv != nullptr) {
        delete ir_recv;
    }
    ir_send = new IRsend(send_pin);
    INFOF("Load IR send pin at %d\n", send_pin);
    ir_send->begin();
    ir_recv = new IRrecv(recv_pin, k_capture_buffer_size, k_timeout, true);
    disableIRIn();
}

void disableIRIn() {
    ir_recv->disableIRIn();
}

void enableIRIn() {
    ir_recv->enableIRIn();
}