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

#include <LittleFS.h>

#include "defines.h"

#include "global.h"
#include "serials.h"
#include "user_settings.h"
#include "iris_client.h"
#include "http_client.h"
#include "utils.h"

#include "ir_emit.h"

#define IR_SERIES_MAX    (1024)
#define IR_END_CODE      (10000)

const uint8_t k_timeout = 50;
// As this program is a special purpose capture/decoder, let us use a larger
// than normal buffer so we can handle Air Conditioner remote codes.
const uint16_t k_capture_buffer_size = IR_SERIES_MAX;
static IRsend * ir_send = nullptr;
static IRrecv * ir_recv = nullptr;

bool sendIR(String file_name) {
    String save_path = SAVE_PATH + file_name;
    if (LittleFS.exists(save_path)) {
        File cache = LittleFS.open(save_path, "r");
        if (!cache) {
            ERRORF("Failed to open %s", save_path.c_str());
            return false;
        }
        Serial.println();
        uint16_t *data_buffer = (uint16_t *)malloc(sizeof(uint16_t) * IR_SERIES_MAX);
        uint16_t length = cache.size() / 2;
        memset(data_buffer, 0x0, IR_SERIES_MAX);
        INFOF("file size = %d\n", cache.size());
        INFOLN();
        cache.readBytes((char *)data_buffer, cache.size());
        ir_recv->disableIRIn();
        ir_send->sendRaw(data_buffer, length, 38);
        ir_recv->enableIRIn();
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
        ir_recv->enableIRIn();
    }
    return true;
}

bool sendCommand(String file_name, int key) {
    String save_path = SAVE_PATH;
    save_path += file_name;
    if (LittleFS.exists(save_path)) {
        File cache = LittleFS.open(save_path, "r");
        if (cache) {
            UINT16 content_size = cache.size();
            DEBUGF("content size = %d\n", content_size);

            if (content_size != 0) {
                UINT8 *content = (UINT8 *)malloc(content_size * sizeof(UINT8));
                cache.seek(0L, fs::SeekSet);
                cache.readBytes((char *)content, content_size);
                ir_binary_open(2, 1, content, content_size);
                UINT16 *user_data = (UINT16 *)malloc(IR_SERIES_MAX * sizeof(UINT16));
                UINT16 data_length = ir_decode(0, user_data, NULL, FALSE);

                DEBUGF("data_length = %d\n", data_length);
                if (LOG_DEBUG) {
                    for (int i = 0; i < data_length; i++)
                        Serial.printf("%d ", *(user_data + i));
                    Serial.println();
                }
                ir_recv->disableIRIn();
                ir_send->sendRaw(user_data, data_length, 38);
                ir_recv->enableIRIn();
                ir_close();
                free(user_data);
                free(content);
            } else
                ERRORF("Open %s is empty\n", save_path.c_str());
        }
        cache.close();
    }
    return true;
}

void sendStatus(String file, t_remote_ac_status status) {
    String save_path = SAVE_PATH + file;
    String url = String(DOWNLOAD_PREFIX) + file + String(DOWNLOAD_SUFFIX);

    if (!LittleFS.exists(save_path)) {
        downLoadFile(url, file, SAVE_PATH);
    }

    if (LittleFS.exists(save_path)) {
        File cache = LittleFS.open(save_path, "r");
        if (cache) {
            UINT16 content_size = cache.size();
            DEBUGF("content size = %d\n", content_size);

            if (content_size != 0) {
                UINT8 *content = (UINT8 *)malloc(content_size * sizeof(UINT8));
                cache.seek(0L, fs::SeekSet);
                cache.readBytes((char *)content, content_size);
                ir_binary_open(REMOTE_CATEGORY_AC, 1, content, content_size);
                UINT16 *user_data = (UINT16 *)malloc(IR_SERIES_MAX * sizeof(UINT16));
                UINT16 data_length = ir_decode(0, user_data, &status, FALSE);

                DEBUGF("data_length = %d\n", data_length);
                ir_recv->disableIRIn();
                ir_send->sendRaw(user_data, data_length, 38);
                ir_recv->enableIRIn();
                ir_close();
                free(user_data);
                free(content);
                saveACStatus(file, status);
            } else
                ERRORF("Open %s is empty\n", save_path.c_str());
        }
        cache.close();
    }
}

void recvIR() {
    decode_results results;
    if (ir_recv->decode(&results)) {
        DEBUGF("raw length = %d\n", results.rawlen - 1);
        String raw_data;
        for (int i = 1; i < results.rawlen; i++) {
            raw_data += String(*(results.rawbuf + i) * kRawTick) + ",";
        }
        ir_recv->resume();
        send_msg_doc.clear();
        send_msg_doc["cmd"] = "record_rt";
        send_msg_doc["params"]["signal"] = "IR";
        send_msg_doc["params"]["length"] = results.rawlen;
        send_msg_doc["params"]["value"] = raw_data.c_str();
        DEBUGLN(raw_data.c_str());
        saveIR(results);
    }
}

bool saveIR(String file_name) {
    String save_path = SAVE_PATH;
    save_path += file_name;
    return LittleFS.rename("/bin/test", save_path);
}

bool saveIR(decode_results& results) {
    String save_path = SAVE_PATH;
    save_path += "test";
    DEBUGF("save raw data as %s\n", save_path.c_str());
    File cache = LittleFS.open(save_path, "w");
    if (!cache) {
        ERRORLN("Failed to create file");
        return false;
    }
    for (size_t i = 0; i < results.rawlen; i++)
        *(results.rawbuf + i) = *(results.rawbuf + i) * kRawTick;
    cache.write((char *)(results.rawbuf + 1), (results.rawlen - 1) * 2);
    cache.close();
    return true;
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
    DEBUGF("Load IR send pin at %d\n", send_pin);
    ir_send->begin();
    ir_recv = new IRrecv(recv_pin, k_capture_buffer_size, k_timeout, true);
    enableIR();
}

void disableIR() {
    ir_recv->disableIRIn();
}

void enableIR() {
    ir_recv->enableIRIn();
}