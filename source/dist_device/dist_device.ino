#include "configs.h"

time_t datetime;

uint8_t sensor_value = 0;
uint8_t device_state = DEVICE_CONNECT;

static void mqtt_publish(void) {

  delay(1000);
  char mqtt_buff[512];
  StaticJsonDocument<200> doc;

  doc["device_state"] = device_state;
  doc["sensor_value"] = sensor_value;

  serializeJson(doc, mqtt_buff);
  device.publish(TOPIC_PUB, mqtt_buff);
}

static uint8_t connect_lan(void) {
//Establish LAN connection.

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  Serial.println("Connecting to LAN...");
  while (WiFi.status() != WL_CONNECTED)
  Serial.println("LAN Connected!");

  return 1;
}

static uint8_t connect_ntp(void) {
//Time syncing with NTP.

  Serial.println("Syncing clock..");
  configTime(2 * 3600, 0 * 3600, NTPSERVER);
  datetime = time(nullptr);

  struct tm timeinfo;
  gmtime_r(&datetime, &timeinfo);
  Serial.println("Clock synced!");

  return 1;
}

static uint8_t connect_aws(void) {
//Establish AWS connection.

  net.setTrustAnchors(&aws_crt);
  net.setClientRSACert(&cli_crt, &cli_key);

  device.setServer(MQTT_HOST, MQTT_PORT);

  Serial.println("Connecting to AWS...");
  while (!device.connect(DEVICE_ID))
  Serial.println("AWS Connected!");

  device.subscribe(TOPIC_SUB);

  return 1;
}

static void sensor_read(void) {
//Read current distance value.

  uint64_t duration = 0;

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH);
  sensor_value = (uint8_t)(duration * SOUND/2);

  delay(500);
}

uint8_t connect_routine(void) {
  
  uint8_t states = 0;
  states += connect_lan();
  states += connect_ntp();
  states += connect_aws();

  if (states == 3)
    return 1;
  return 0;
}

uint8_t ongoing_routine(void) {

  sensor_read();
  mqtt_publish();
  if (!device.connected())
    return 0;
  return 1;
}

static void device_driver(void) {

  switch(device_state) {

    case DEVICE_CONNECT:
      device_state = connect_routine() ? DEVICE_ONGOING : DEVICE_CONNECT;
    break;

    case DEVICE_ONGOING:
      device_state = ongoing_routine() ? DEVICE_ONGOING : DEVICE_CONNECT;
    break;
  }
}

void setup() {
  
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

void loop() {

  device_driver();
}
/*
{
  "device_state": 0,
  "vessel_state": 80,
  "sensor_value": 1013,
  "update_event" : 1,
  "treshold_yel": 20,
  "treshold_red": 20,
}
*/
