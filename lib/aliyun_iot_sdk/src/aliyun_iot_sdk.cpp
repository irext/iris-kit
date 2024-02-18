/**
 *
 *  Filename:      aliyun_iot_sdk.cpp
 *
 *  Description:   basic SDK for ESP32
 *
 *  Created by strawmanbobi 2022-01-03
 *
 *  Copyright (c) 2016-2022 IRext
 *
 **/

#include <PubSubClient.h>
#include <SHA256.h>

#include "aliyun_iot_sdk.h"

#define CHECK_INTERVAL          (30000)
#define MESSAGE_BUFFER_SIZE     (10)
#define MQTT_CONNECT_RETRY_MAX  (3)

static const char *deviceName = NULL;
static const char *productKey = NULL;
static const char *deviceSecret = NULL;
static const char *region = NULL;
static const char *iotInstanceId = NULL;

struct DeviceProperty {
    String key;
    String value;
};

DeviceProperty PropertyMessageBuffer[MESSAGE_BUFFER_SIZE];

#define SHA256HMAC_SIZE 32
#define DATA_CALLBACK_SIZE 20

#define MQTT_WAIT_GENERIC  (10000)

#define ALINK_BODY_FORMAT "{\"id\":\"123\",\"version\":\"1.0\",\"method\":\"thing.event.property.post\",\"params\":%s}"
#define ALINK_EVENT_BODY_FORMAT "{\"id\": \"123\",\"version\": \"1.0\",\"params\": %s,\"method\": \"thing.event.%s.post\"}"

static unsigned long lastMs = 0;

static PubSubClient *client = NULL;

// bind callbacks, support at most 28 callbacks
static pointerDesc pointerArray[20];
static pPointerDesc pPointerArray;

char AliyunIoTSDK::clientId[CLIENT_ID_MAX_LEN] = "";
char AliyunIoTSDK::mqttUsername[USER_NAME_MAX_LEN] = "";
char AliyunIoTSDK::mqttPwd[PASSWORD_MAX_LEN] = "";
char AliyunIoTSDK::domain[DOMAIN_NAME_MAX_LEN] = "";

char AliyunIoTSDK::ALINK_TOPIC_PROP_POST[150] = "";
char AliyunIoTSDK::ALINK_TOPIC_PROP_SET[150] = "";
char AliyunIoTSDK::ALINK_TOPIC_EVENT[150] = "";

static long long getTimeMills(void) {
    static uint32_t low32, high32;
    uint32_t new_low32 = millis();
    if (new_low32 < low32) high32++;
    low32 = new_low32;
    return (uint64_t) high32 << 32 | low32;
}

static void parmPass(JsonVariant parm) {
    for (int i = 0; i < DATA_CALLBACK_SIZE; i++) {
        if (pointerArray[i].key) {
            bool hasKey = parm["params"].containsKey(pointerArray[i].key);
            if (hasKey) {
                pointerArray[i].fp(parm["params"]);
            }
        }
    }
}

static void callback(char *topic, byte *payload, unsigned int length) {
    Serial.print("INFO\tMessage arrived [");
    Serial.print(topic);
    Serial.print("] ");
    payload[length] = '\0';
    Serial.println((char *)payload);

    if (strstr(topic, AliyunIoTSDK::ALINK_TOPIC_PROP_SET)) {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
            parmPass(doc.as<JsonVariant>());
        }
    }
}

static bool mqttConnecting = false;
int AliyunIoTSDK::mqttCheckConnect() {
    int mqttStatus = 0;
    int connectRetry = 0;

    Serial.println("INFO:\tAlink MQTT connection checking...");

    if (client != NULL && false == mqttConnecting) {
        Serial.print("INFO:\tAlink MQTT client state = ");
        Serial.println(client->state());
        if (MQTT_CONNECTED != client->state()) {
            connectRetry = 0;
            while (false == client->connected()) {
                client->disconnect();
                Serial.print("INFO:\tConnecting to MQTT Server, clientId = ");
                Serial.print(clientId);
                Serial.print(", mqttUserName = ");
                Serial.print(mqttUsername);
                Serial.print(", mqttPwd = ");
                Serial.println(mqttPwd);
                mqttConnecting = true;
                if (client->connect(clientId, mqttUsername, mqttPwd)) {
                    Serial.println("INFO:\tMQTT Connected!");
                } else {
                    Serial.print("ERROR:\tMQTT Connect err : ");
                    Serial.println(client->state());
                    Serial.print("ERROR:\tusername : ");
                    Serial.println(mqttUsername);
                    Serial.print("ERROR:\tpassword : ");
                    Serial.println(mqttPwd);
                    delay(MQTT_WAIT_GENERIC);
                    connectRetry++;
                    Serial.print("INFO:\tretry : ");
                    Serial.println(connectRetry);
                    mqttStatus = -1;
                    if (connectRetry > MQTT_CONNECT_RETRY_MAX) {
                        Serial.println("ERROR:\tmax connect retry times reached");
                        break;
                    }
                }
                mqttConnecting = false;
            }
        }
    }
    return mqttStatus;
}

int AliyunIoTSDK::begin(PubSubClient &mqttClient,
                         const char *_clientId,
                         const char *_productKey,
                         const char *_deviceName,
                         const char *_deviceSecret,
                         const char *_iotInstanceId,
                         const char *_region) {
    client = new PubSubClient(mqttClient);
    productKey = _productKey;
    deviceName = _deviceName;
    deviceSecret = _deviceSecret;
    iotInstanceId = _iotInstanceId;
    region = _region;
    uint16_t port = 443;

    int res = 0;

    res = aliot_mqtt_sign(productKey, deviceName, deviceSecret, _clientId,
        clientId, CLIENT_ID_MAX_LEN, mqttUsername, USER_NAME_MAX_LEN, mqttPwd, PASSWORD_MAX_LEN);

    if (0 != res) {
        Serial.println("ERROR\tfailed to sign aliot mqtt params");
        return -1;
    }
    Serial.print("DEBUG\tMQTT clietnId = ");
    Serial.println(clientId);
    Serial.print("DEBUG\tMQTT userName = ");
    Serial.println(mqttUsername);
    Serial.print("DEBUG\tMQTT password = ");
    Serial.println(mqttPwd);

    sprintf(ALINK_TOPIC_PROP_POST, "/sys/%s/%s/thing/event/property/post", productKey, deviceName);
    sprintf(ALINK_TOPIC_PROP_SET, "/sys/%s/%s/thing/service/property/set", productKey, deviceName);
    sprintf(ALINK_TOPIC_EVENT, "/sys/%s/%s/thing/event", productKey, deviceName);

    if (NULL != iotInstanceId) {
        sprintf(domain, "%s.mqtt.iothub.aliyuncs.com", iotInstanceId);
        port = 1883;
    } else {
        sprintf(domain, "%s.iot-as-mqtt.%s.aliyuncs.com", productKey, region);
        port = 1883;
    }
    
    Serial.print("INFO\tconnect to aliyun : ");
    Serial.print(domain);
    Serial.print(":");
    Serial.println(port);
    client->setServer(domain, port);

#if defined USE_STANDARD_THING_MODEL_TOPIC
    client->setCallback(callback);
#endif
    Serial.print("INFO\tconnection check in begin\n");
    return mqttCheckConnect();
}

int AliyunIoTSDK::loop() {
    int mqttStatus = 0;
    client->loop();
    if (millis() - lastMs >= CHECK_INTERVAL) {
        lastMs = millis();
        mqttStatus = mqttCheckConnect();
    }

    if (0 == mqttStatus) {
        messageBufferCheck();
    }

    return mqttStatus;
}

void AliyunIoTSDK::sendEvent(const char *eventId, const char *param) {
    char topicKey[156];
    snprintf(topicKey, sizeof(topicKey) - 1, "%d/%s/post", 0, eventId);
    char jsonBuf[1024];
    sprintf(jsonBuf, ALINK_EVENT_BODY_FORMAT, param, eventId);
    Serial.print("INFO\tsend : ");
    Serial.println(jsonBuf);
    boolean d = client->publish(topicKey, jsonBuf);
    Serial.print("INFO\tpublish : 0 successfully : ");
    Serial.println(d);
}

void AliyunIoTSDK::sendEvent(const char *eventId) {
    sendEvent(eventId, "{}");
}

void AliyunIoTSDK::sendCustom(const char *topic, const char *eventBody) {
    boolean d = client->publish(topic, eventBody);
    Serial.print("INFO\tpublish : 0 sucessfully : ");
    Serial.println(d);
}

void AliyunIoTSDK::sendCustomData(const char *topic, const uint8_t *data, int length) {
    boolean d = client->publish(topic, data, length);
    Serial.print("INFO\tpublish : 0 sucessfully : ");
    Serial.println(d);
}

boolean AliyunIoTSDK::subscribe(const char* topic, int qos) {
    return client->subscribe(topic, qos);
}

void AliyunIoTSDK::registerCustomCallback(MQTT_CALLBACK_SIGNATURE) {
    client->setCallback(callback);
}

unsigned long lastSendMS = 0;

// check data sending buffer
void AliyunIoTSDK::messageBufferCheck() {
    int bufferSize = 0;
    for (int i = 0; i < MESSAGE_BUFFER_SIZE; i++) {
        if (PropertyMessageBuffer[i].key.length() > 0) {
            bufferSize++;
        }
    }
    if (bufferSize > 0) {
        if (bufferSize >= MESSAGE_BUFFER_SIZE) {
            sendBuffer();
        } else {
            unsigned long nowMS = millis();
            // send every 5 seconds
            if (nowMS - lastSendMS > 5000) {
                sendBuffer();
                lastSendMS = nowMS;
            }
        }
    }
}

// send data in buffer
void AliyunIoTSDK::sendBuffer() {
    int i;
    String buffer;
    for (i = 0; i < MESSAGE_BUFFER_SIZE; i++) {
        if (PropertyMessageBuffer[i].key.length() > 0) {
            buffer += "\"" + PropertyMessageBuffer[i].key + "\":" + PropertyMessageBuffer[i].value + ",";
            PropertyMessageBuffer[i].key = "";
            PropertyMessageBuffer[i].value = "";
        }
    }

    buffer = "{" + buffer.substring(0, buffer.length() - 1) + "}";
    send(buffer.c_str());
}

void addMessageToBuffer(char *key, String value) {
    int i;
    for (i = 0; i < MESSAGE_BUFFER_SIZE; i++) {
        if (PropertyMessageBuffer[i].key.length() == 0) {
            PropertyMessageBuffer[i].key = key;
            PropertyMessageBuffer[i].value = value;
            break;
        }
    }
}

void AliyunIoTSDK::send(const char *param) {
    char jsonBuf[1024];
    sprintf(jsonBuf, ALINK_BODY_FORMAT, param);
    Serial.print("INFO\tsend : ");
    Serial.println(jsonBuf);
    boolean d = client->publish(ALINK_TOPIC_PROP_POST, jsonBuf);
    Serial.print("INFO\tpublish : 0 sucessfully : ");
    Serial.println(d);
}

void AliyunIoTSDK::send(char *key, float number) {
    addMessageToBuffer(key, String(number));
    messageBufferCheck();
}

void AliyunIoTSDK::send(char *key, int number) {
    addMessageToBuffer(key, String(number));
    messageBufferCheck();
}

void AliyunIoTSDK::send(char *key, double number) {
    addMessageToBuffer(key, String(number));
    messageBufferCheck();
}

void AliyunIoTSDK::send(char *key, char *text) {
    addMessageToBuffer(key, "\"" + String(text) + "\"");
    messageBufferCheck();
}

#if defined USE_STANDARD_THING_MODEL_TOPIC
int AliyunIoTSDK::bindData(char *key, pFuncPointer fp) {
    int i;
    for (i = 0; i < DATA_CALLBACK_SIZE; i++) {
        if (!pointerArray[i].fp) {
            pointerArray[i].key = key;
            pointerArray[i].fp = fp;
            return 0;
        }
    }
    return -1;
}

int AliyunIoTSDK::unbindData(char *key) {
    int i;
    for (i = 0; i < DATA_CALLBACK_SIZE; i++) {
        if (!strcmp(pointerArray[i].key, key)) {
            pointerArray[i].key = NULL;
            pointerArray[i].fp = NULL;
            return 0;
        }
    }
    return -1;
}
#endif
