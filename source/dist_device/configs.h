#ifndef CONFIGS__H
#define CONFIGS__H

#include "secrets.h"
#include <time.h>
#include <stdint.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

#define TRIG_PIN D1 // Trig Pin (HC-) Wemos D1 mini (ESP8266).
#define ECHO_PIN D2 // Echo Pin (HC-) Wemos D1 mini (ESP8266).
#define SOUND 0.034 // Speed of sound in room temperature. Used for calculate distance.

#define DEVICE_ID "device_1"
#define TOPIC_PUB "device/device_1/data"
#define TOPIC_SUB "device/device_1/data"
#define NTPSERVER "pool.ntp.org"
#define MQTT_PORT 8883
#define MQTT_HOST "a1qt5qae1xcxwq-ats.iot.eu-north-1.amazonaws.com"

#define DEVICE_CONNECT 0  // Connect to Local Area Network
#define DEVICE_ONGOING 1

#define EVENT_TIMER 2000

BearSSL::X509List aws_crt(AWS_CRT);
BearSSL::X509List cli_crt(DEV_CRT);
BearSSL::PrivateKey cli_key(DEV_KEY);

WiFiClientSecure net;
PubSubClient device(net);

#endif