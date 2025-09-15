/**
 *
 *  Filename:      AliyunIoTSDK.h
 *
 *  Description:   header file of basic SDK for ESP32
 *
 *  Created by strawmanbobi 2022-01-03
 *
 *  Copyright (c) 2016-2022 IRext
 *
 **/

#ifndef ALIYUN_IOT_SDK_H
#define ALIYUN_IOT_SDK_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "Client.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*funcPointer)(JsonVariant ele);

typedef struct {
  char *key;
  funcPointer fp;
} pointerDesc, *pPointerDesc;

#ifdef __cplusplus
}
#endif

class AliyunIoTSDK {
private:
  // MQTT client related parameters
  static char mqttPwd[256];
  static char clientId[256];
  static char mqttUsername[100];
  static char domain[150];

  static void messageBufferCheck();
  static void sendBuffer();
public:

  // MQTT keep alive handler
  static int mqttCheckConnect();

  // offical defined topic templates (not used)
  static char ALINK_TOPIC_PROP_POST[150];
  static char ALINK_TOPIC_PROP_SET[150];
  static char ALINK_TOPIC_EVENT[150];

  // MQTT keep alive task
  static int loop();

  /**
   * Initialize and connect to AliyunIoT
   * @param espClient : WiFi client
   * @param _productKey : AliyunIoT product key
   * @param _deviceName : AliyunIoT device name
   * @param _deviceSecret : AliyunIoT device secret
   * @param _region : AliyunIoT region
   */
  static int begin(PubSubClient &mqtt_client,
                   const char *_productKey,
                   const char *_deviceName,
                   const char *_deviceSecret,
                   const char *_region);

  /**
   * Send data
   * @param param : JSON formated string with key and value : {"${key}":"${value}"}
   */
  static void send(const char *param);

  /**
   * Send single data in float
   * @param key : key
   * @param number : value
   */
  static void send(char *key, float number);

  /**
   * Send single data in integer
   * @param key : key
   * @param number : value
   */
  static void send(char *key, int number);

  /**
   * Send single data in double
   * @param key : key
   * @param number : value
   */
  static void send(char *key, double number);

  /**
   * Send single data in string
   * @param key : key
   * @param text : value
   */
  static void send(char *key, char *text);

  /**
   * Send standard thing model data
   * @param eventId : eventId predefined in AliyunIoT
   * @param param : JSON formated string with key and value : {"${key}":"${value}"}
   */
  static void sendEvent(const char *eventId, const char *param);

  /**
   * Send empty thing model data
   * @param eventId : eventId predefined in AliyunIoT
   */
  static void sendEvent(const char *eventId);

  /**
   * Subscribe MQTT topic for Aliot
   *
   * @param topic : topic in string
   * @param qos : MQTT qos param
   * @return if succeeded
   */
  static boolean subscribe(const char* topic, int qos);

  /**
   * Register customized MQTT message callback
   *
   * @param callback : callback pointer
   */
  static void registerCustomCallback(MQTT_CALLBACK_SIGNATURE);

#if defined USE_STANDARD_THING_MODEL_TOPIC
  /**
   * Register callback for downstream MQTT message
   */
  static void bind(MQTT_CALLBACK_SIGNATURE);

  /**
   * Register callback for downstream MQTT message with specific eventId
   * @param eventId : eventId predefined in AliyunIoT
   */
  static void bindEvent(const char * eventId, MQTT_CALLBACK_SIGNATURE);

  /**
   * Register callback for downstream MQTT message with specific key
   * @param key : key predefined in thing model
   */
  static int bindData(char *key, funcPointer fp);

  /**
   * Unregister callback for specified key
   * @param key : key predefined in thing model
   */
  static int unbindData(char *key);
#endif

};
#endif /* ALIYUN_IOT_SDK_H */
