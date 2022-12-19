#ifndef CONFIGS__H
#define CONFIGS__H

#include "secrets.h"
#include <time.h>
#include <stdint.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

#define TRIG_PIN D1 // Trig Pin (HC-SR05), Wemos D1 mini (ESP8266).
#define ECHO_PIN D2 // Echo Pin (HC-SR05), Wemos D1 mini (ESP8266).
#define SOUND 0.034 // Speed of sound in room temperature. Used for calculate distances.

#define DEVICE_ID "mdevice"
#define TOPIC_PUB "solna_hagalund/frosunda/room1/vessel1/mdevice/pub"
#define TOPIC_SUB "solna_hagalund/frosunda/room1/vessel1/mdevice/sub"
#define NTPSERVER "pool.ntp.org"
#define MQTT_PORT 8883
#define MQTT_HOST "a1qt5qae1xcxwq-ats.iot.eu-north-1.amazonaws.com"

#define TEMP_API "http://api.temperatur.nu/tnu_1.17.php?p=solna_hagalund&cli=api_demo"

#define DEVICE_CONNECT 0  // Connect to the cloud.
#define DEVICE_CONFIGS 1  // Wait for threshold values from lazy EMC employes.
#define DEVICE_ONGOING 2  // Read, evaluate. Repeat.

#define VESSEL_GRN 0
#define VESSEL_YEL 1
#define VESSEL_RED 2

#define EVENT_TIMER 1000 * 30     // Half a minute is given to check if triggered "threshold action" was fake or not.
#define API_TIMER 1000 * 60 * 240 // Read temp from API every 4 hour.

BearSSL::X509List aws_crt(AWS_CRT);
BearSSL::X509List cli_crt(DEV_CRT);
BearSSL::PrivateKey cli_key(DEV_KEY);

WiFiClient api;
HTTPClient http;

WiFiClientSecure net;
PubSubClient device(net);

// Used during code tests with AWS MQTT test client. Sends "vessel configs" back to the device.

/*
{
  threshold_TMP : 30,
  threshold_YEL : 20,
  threshold_RED : 5
}

+/+/+/+/+/pub
solna_hagalund/frosunda/room1/vessel1/mdevice/sub
*/
#endif
