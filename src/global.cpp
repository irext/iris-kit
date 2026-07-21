/**
 *
 * Copyright (c) 2020-2026 IRext Opensource Organization
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

#include "defines.h"

#include "global.h"

StaticJsonDocument<1024> http_request_doc;
StaticJsonDocument<1024> http_response_doc;

// out solicited request message
StaticJsonDocument<512> mqtt_upstream_topic_msg_doc;

// out solicited response message
StaticJsonDocument<512> mqtt_upstream_topic_rsp_doc;

// out unsolicited message
StaticJsonDocument<2048> mqtt_upstream_topic_ind_doc;

// out heartbeat message
StaticJsonDocument<512> mqtt_upstream_topic_hbt_doc;

// in message
StaticJsonDocument<512> mqtt_downstream_topic_msg_doc;

// inner json data
StaticJsonDocument<512> status_notify_doc;
StaticJsonDocument<2048> emit_code_doc;

// IR json data
StaticJsonDocument<1024> recv_msg_doc;
StaticJsonDocument<1024> send_msg_doc;


WiFiManager wifi_manager;
WiFiClient wifi_client;

uint8_t ir_send_pin = T_IR;
uint8_t ir_receive_pin = R_IR;

#ifdef USE_RF
    uint8_t rf315_send_pin = T_315;
    uint8_t rf315_receive_pin = R_315;
    uint8_t rf433_send_pin = T_433;
    uint8_t rf433_receive_pin = R_433;
#endif
