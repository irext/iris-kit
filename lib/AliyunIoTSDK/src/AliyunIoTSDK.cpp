/**
 *
 *  Filename:      AliyunIoTSDK.cpp
 *
 *  Description:   basic SDK for ESP32
 *
 *  Created by strawmanbobi 2022-01-03
 *
 *  Copyright (c) 2016-2022 IRext
 *
 **/

#include "AliyunIoTSDK.h"
#include <PubSubClient.h>
#include <SHA256.h>

#define CHECK_INTERVAL 10000
#define MESSAGE_BUFFER_SIZE 10
#define MQTT_CONNECT_RETRY_MAX 3

static const char *deviceName = NULL;
static const char *productKey = NULL;
static const char *deviceSecret = NULL;
static const char *region = NULL;

struct DeviceProperty {
    String key;
    String value;
};

DeviceProperty PropertyMessageBuffer[MESSAGE_BUFFER_SIZE];

#define MQTT_PORT 1883

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

char AliyunIoTSDK::clientId[256] = "";
char AliyunIoTSDK::mqttUsername[100] = "";
char AliyunIoTSDK::mqttPwd[256] = "";
char AliyunIoTSDK::domain[150] = "";

char AliyunIoTSDK::ALINK_TOPIC_PROP_POST[150] = "";
char AliyunIoTSDK::ALINK_TOPIC_PROP_SET[150] = "";
char AliyunIoTSDK::ALINK_TOPIC_EVENT[150] = "";

static String hmac256(const String &signcontent, const String &ds) {
    byte hashCode[SHA256HMAC_SIZE];
    SHA256 sha256;

    const char *key = ds.c_str();
    size_t keySize = ds.length();

    sha256.resetHMAC(key, keySize);
    sha256.update((const byte *)signcontent.c_str(), signcontent.length());
    sha256.finalizeHMAC(key, keySize, hashCode, sizeof(hashCode));

    String sign = "";
    for (byte i = 0; i < SHA256HMAC_SIZE; ++i) {
        sign += "0123456789ABCDEF"[hashCode[i] >> 4];
        sign += "0123456789ABCDEF"[hashCode[i] & 0xf];
    }

    return sign;
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
    Serial.print("Message arrived [");
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
                    Serial.print("ERROR:\tMQTT Connect err: ");
                    Serial.println(client->state());
                    delay(MQTT_WAIT_GENERIC);
                    connectRetry++;
                    Serial.print("INFO:\tretry: ");
                    Serial.println(connectRetry);
                    mqttStatus = -1;
                    if (connectRetry > MQTT_CONNECT_RETRY_MAX) {
                        Serial.println("ERROR:\t max connect retry times reached");
                        break;
                    }
                }
                mqttConnecting = false;
            }
        }
    }
    return mqttStatus;
}

int AliyunIoTSDK::begin(Client &espClient,
                         const char *_productKey,
                         const char *_deviceName,
                         const char *_deviceSecret,
                         const char *_region) {
    client = new PubSubClient(espClient);
    productKey = _productKey;
    deviceName = _deviceName;
    deviceSecret = _deviceSecret;
    region = _region;
    long times = millis();
    String timestamp = String(times);

    sprintf(clientId, "%s|securemode=3,signmethod=hmacsha256,timestamp=%s|", deviceName,
            timestamp.c_str());

    String signcontent = "clientId";
    signcontent += deviceName;
    signcontent += "deviceName";
    signcontent += deviceName;
    signcontent += "productKey";
    signcontent += productKey;
    signcontent += "timestamp";
    signcontent += timestamp;

    String pwd = hmac256(signcontent, deviceSecret);

    strcpy(mqttPwd, pwd.c_str());

    sprintf(mqttUsername, "%s&%s", deviceName, productKey);
    sprintf(ALINK_TOPIC_PROP_POST, "/sys/%s/%s/thing/event/property/post", productKey, deviceName);
    sprintf(ALINK_TOPIC_PROP_SET, "/sys/%s/%s/thing/service/property/set", productKey, deviceName);
    sprintf(ALINK_TOPIC_EVENT, "/sys/%s/%s/thing/event", productKey, deviceName);

    sprintf(domain, "%s.iot-as-mqtt.%s.aliyuncs.com", productKey, region);
    client->setServer(domain, MQTT_PORT);

#if defined USE_STANDARD_THING_MODEL_TOPIC
    client->setCallback(callback);
#endif

    return mqttCheckConnect();
}

int AliyunIoTSDK::loop() {
    int mqttStatus = 0;
    client->loop();
    if (millis() - lastMs >= CHECK_INTERVAL) {
        lastMs = millis();
        mqttStatus = mqttCheckConnect();
    }
    Serial.print("MQTT connect return: ");
    Serial.println(mqttStatus);
 
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
    Serial.println(jsonBuf);
    boolean d = client->publish(topicKey, jsonBuf);
    Serial.print("publish: 0 successfully: ");
    Serial.println(d);
}

void AliyunIoTSDK::sendEvent(const char *eventId) {
    sendEvent(eventId, "{}");
}

void AliyunIoTSDK::sendCustom(const char *topic, const char *eventBody) {
    boolean d = client->publish(topic, eventBody);
    Serial.print("publish:0 sucessfully:");
    Serial.println(d);
}

void AliyunIoTSDK::sendCustomData(const char *topic, const uint8_t *data, int length) {
    boolean d = client->publish(topic, data, length);
    Serial.print("publish:0 sucessfully:");
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
    // Serial.println("bufferSize:");
    // Serial.println(bufferSize);
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
    Serial.println(jsonBuf);
    boolean d = client->publish(ALINK_TOPIC_PROP_POST, jsonBuf);
    Serial.print("publish:0 sucessfully:");
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
