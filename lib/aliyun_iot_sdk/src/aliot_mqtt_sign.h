/**
 *
 * Copyright (c) 2020-2024 IRbaby-IRext
 *
 * Author: Strawmanbobi and Caffreyfans
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

#include <Arduino.h>

#ifndef ALIOT_MQTT_SIGN_H
#define ALIOT_MQTT_SIGN_H

#define PRODUCT_KEY_MAX_LEN           (20)
#define DEVICE_NAME_MAX_LEN           (32)
#define DEVICE_SECRET_MAX_LEN         (64)

#define SIGN_SOURCE_MAX_LEN           (256)
#define CLIENT_ID_MAX_LEN             (256)
#define USER_NAME_MAX_LEN             (256)
#define PASSWORD_MAX_LEN              (65)

#define TIMESTAMP_VALUE             "1708225691253"
#define MQTT_CLINETID_KV            "|securemode=2,signmethod=hmacsha256,timestamp=1708225691253|"


#ifdef __cplusplus
extern "C" {
#endif

int aliot_mqtt_sign(const char *product_key, const char *device_name, const char *device_secret, const char* device_id,
                    char* client_id, int max_client_id_size, char* user_name, int max_user_name_size, char* password, int max_password_size);


#ifdef __cplusplus
}
#endif

#endif // ALIOT_MQTT_SIGN_H