#include "configs.h"

// I've put all functions in this .ini-file because I don't really know how to import logic
// from my own custom made modules within the Arduino ecosystem without being yelled at by
// either the IDE or the compiler, or both.

// This flash runs in yolo-mode with no serious error-handling in place at all yet. I've
// utilized stdint.h mainly because of my kink for the uint8_t type.

time_t datetime;

uint16_t sensor_value = 0;               // Current sample (published in 1 sec intervals).

uint8_t update_event  = 0;               // If set, change in vessel state has occured.  
uint8_t heatwv_event  = 0;               // If set, heatwave/high temp has been recorded.

uint8_t threshold_TMP = 0;               // threshold value and definition of high temp.
uint8_t threshold_YEL = 0;               // Threshold value for VESSEL_YEL.
uint8_t threshold_RED = 0;               // Threshold value for VESSEL_RED.

float temperature_API = 0;               // Last fetched temp value.

uint8_t vessel_state  = VESSEL_GRN;
uint8_t device_state  = DEVICE_CONNECT;

static void mqtt_publish(void) {

// Publish 'vessel_state', 'sensor_value', 'update_event' and temp to the cloud.

  delay(1000);
  char mqtt_buff[512];
  StaticJsonDocument<200> doc;

  doc["vessel_state"] = vessel_state;
  doc["sensor_value"] = sensor_value;
  doc["update_event"] = update_event;
  doc["tmpapi_value"] = temperature_API;


  serializeJson(doc, mqtt_buff);
  device.publish(TOPIC_PUB, mqtt_buff);

  update_event = 0;
}

static void mqtt_receive(char *topic, byte *payload, unsigned int length) {
// As for now, the only thing possible to receive from the cloud is threshold values.

  if (length == 0) return; 

  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);

  threshold_TMP = doc["threshold_TMP"];
  threshold_YEL = doc["threshold_YEL"];
  threshold_RED = doc["threshold_RED"];
}

static uint8_t connect_lan(void) {
// Establish LAN connection.

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("connecting to LAN...");
    delay(2000);
  }
  Serial.println("connected to LAN!");
  return 1;
}

static uint8_t connect_ntp(void) {
// Time syncing with NTP.

  configTime(2 * 3600, 0 * 3600, NTPSERVER);
  datetime = time(nullptr);

  struct tm timeinfo;
  gmtime_r(&datetime, &timeinfo);

  return 1;
}

static uint8_t connect_aws(void) {
// Establish AWS connection.

  net.setTrustAnchors(&aws_crt);
  net.setClientRSACert(&cli_crt, &cli_key);

  device.setServer(MQTT_HOST, MQTT_PORT);
  device.setCallback(mqtt_receive);
  
  while (!device.connect(DEVICE_ID)) {
    Serial.println("connecting to AWS...");
    delay(2000);
  }
  Serial.println("connected to AWS!");

  device.subscribe(TOPIC_SUB);

  return 1;
}

static uint8_t tempget_api(void) {

  DynamicJsonDocument doc(2048);

  Serial.println("reading temp from API...");
  http.useHTTP10(true);
  http.begin(api, TEMP_API);
  
  http.GET();

  deserializeJson(doc, http.getStream());
  temperature_API = doc["stations"][0]["temp"].as<float>();
  
  http.end();

  Serial.print("current temperature: ");
  Serial.println(temperature_API);

  return 1;
}

static uint8_t connect_routine(void) {
// "Wrapper" for connectivity-logic. 'state-values' in place for error-handling later on.

  uint8_t conn_states = 0;

  conn_states += connect_lan();
  conn_states += connect_ntp();
  conn_states += connect_aws();
  conn_states += tempget_api();

  if (conn_states == 4)
    return 1;
  return 0;
}

static uint8_t configs_routine(void) {
// After the sensor has been attached to a vessel, is up and running and connected to the cloud,
// it's up to WMC:s employees to "config" threshold values for it. As long these values hasn't
// been set, the device won't jump to state "DEVICE_ONGOING" and send sensor values.

  device.loop();
  if (threshold_TMP < 1 || threshold_YEL < 1 || threshold_RED < 1 ) {
    Serial.println("waiting for threshold configs...");
    delay(2000);
    return 0;
  }
  Serial.println("configs received!");

  return 1;
}

static void sensor_read(void) {
// Reads current 'sensor_value'.

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

static void vessel_eval(void) {
// Assigns 'vessel_value' based on sensor value relative to given thresholds. 

  if (sensor_value <= threshold_RED) {        // If sensor_val lesser or equal red, empty...
    vessel_state = VESSEL_RED;
  } else if (sensor_value > threshold_YEL) {  // else if larger than yel, it is green.
    vessel_state = VESSEL_GRN;
  } else {
    vessel_state = VESSEL_YEL;                // else yellow.
  }
}

static void update_eval(void) {
// Evaluates/checks if last 'sensor_value' has reached/passed a threshold. If so, this might
// just be temporarly as people throwing stuff or the sensor has catched a fly. I've
// implemented a 30 sec 'event_buffer' for this reason. If 'last_vessel_state' still
// differ from 'vessel_state' after this period, a message with the 'update' attribute
// set will be published to the cloud.

  static uint64_t event_time_stamp   = 0;
  static uint8_t  prev_vessel_state  = 0;
  
  if (event_time_stamp) {
    if (millis() - event_time_stamp < EVENT_TIMER)
      return;

    update_event = (prev_vessel_state != vessel_state) ? 1 : 0;
    prev_vessel_state = vessel_state;    
    event_time_stamp = 0;

    Serial.println("event ongoing: exit event timer.");
    
    if (update_event)
      Serial.println("updating vessel_state.");
    else
      Serial.println("false alarm.");

  } else if (vessel_state != prev_vessel_state) {
      Serial.println("event ongoing: init event timer.");
      event_time_stamp = millis();

  } else {
    prev_vessel_state = vessel_state;
  }
}

static uint8_t ongoing_routine(void) {
// "Wrapper" for "ongoing-logic".

  sensor_read();
  vessel_eval();
  update_eval();
  mqtt_publish();

  if (!device.connected())
    return 0;
  return 1;
}

static void device_driver(void) {
// Rather optimistic main driver/statemachine.

  switch(device_state) {

    case DEVICE_CONNECT:    
      device_state = connect_routine() ? DEVICE_CONFIGS : DEVICE_CONNECT;
    break;

    case DEVICE_CONFIGS:    
      device_state = configs_routine() ? DEVICE_ONGOING : DEVICE_CONFIGS;    
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
