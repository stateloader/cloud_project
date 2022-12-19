#include "configs.h"

// I've put all functions in this .ini-file because I don't really know how to import logic from
// my own custom made modules within the Arduino ecosystem, at least not without being yelled at
// by either the IDE or the compiler, or both.

// This flash runs in yolo-mode with no serious error-handling in place at all.

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
// Publish 'vessel_state', 'sensor_value', 'update_event', 'tmpapi_value' and 'heatwv_event'
// to the cloud.

  delay(1000);
  char mqtt_buff[512];
  StaticJsonDocument<200> doc;

  doc["vessel_state"] = vessel_state;
  doc["sensor_value"] = sensor_value;
  doc["update_event"] = update_event;
  doc["tmpapi_value"] = temperature_API;
  doc["heatwv_event"] = heatwv_event;

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
// Calls API (temperatur.nu) and fetches temp value from a sensor close to where I'm
// living (i.e: "the vessel").

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
// "Wrapper" for connectivity-logic. 'retval' in place for implementaton of error handeling
// later on.

  uint8_t retval = 0;

  retval += connect_lan();
  retval += connect_ntp();
  retval += connect_aws();
  retval += tempget_api();

  if (retval == 4)
    return 1;
  return 0;
}

static uint8_t configs_routine(void) {
// After a sensor has been attached to a vessel, is up and running and connected to the cloud,
// it's up to "WMC:s employees" to "config" the threshold values. While these values hasn't
// been set, the device won't jump to state "DEVICE_ONGOING" and send sensor values.

  device.loop(); // C++ magic. If a MQTT message is received there will be a callback.

  if (threshold_TMP < 1 || threshold_YEL < 1 || threshold_RED < 1 ) {
    Serial.println("waiting for threshold configs...");
    delay(2000);
    return 0;
  }
  Serial.println("configs received!");

  return 1;
}

static void sensor_read(void) {
// Read current 'sensor_value'.

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
// Assigns 'vessel_value' based on 'sensor_value' relative to given thresholds.

// If sensor_val is lesser or equal "red", distance to the waste is very low (i.e: full).
// Else if sensor_val is lesser than "yellow", distance to the waste is very high (i.e: empty).
// Else, the sensor's value is within the boundaries for state "yellow" (i.e: half full).

  heatwv_event = (temperature_API >= threshold_TMP) ? 1 : 0; // Trinary check if "high temp mode".
                                                             // If so, set 'heatwv_event'.
  if (sensor_value <= threshold_RED) {
    vessel_state = VESSEL_RED;
  } else if (sensor_value > threshold_YEL) {
    vessel_state = VESSEL_GRN;
  } else {
    // If the API has sent a temp >= 'treshold_TMP', there's no yellow state at all until the api
    // sends a temp_value < 'treshold_TMP'.
    if (!heatwv_event) {
      vessel_state = VESSEL_YEL;
    } else {
      vessel_state = VESSEL_RED;
    }
  }
}

static void update_eval(void) {
// Evaluates/checks if last 'sensor_value' has reached/passed a threshold. If so, this might just be
// temporary as people throwing stuff or the sensor has catched a fly.

// For this reason, I've implemented a 30 sec 'event_buffer'. If 'last_vessel_state' still differ
// from 'vessel_state' after this period, a message with the 'update' attribute set will be published
// to the cloud, in turn triggering action among used AWS services.

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

static void getapi_eval(void) {
// As for now, variable 'init_stamp' stores a time stamp from last api call. If
// 'millis()' - this value equals or is grather than API_TIMER, do another call.

  static uint64_t init_stamp = 0;
  
  if (init_stamp) {
    if (millis() - init_stamp >= API_TIMER) {
      init_stamp = millis();
      tempget_api();
    }
  } else {
    init_stamp = millis();
  }
}

static uint8_t ongoing_routine(void) {
// "Wrapper" for "ongoing-logic".

  sensor_read();    // read sensor value.
  getapi_eval();    // check timer for api call.
  vessel_eval();    // check thresholds for 'vessel_state' updates (starts timer if so).
  update_eval();    // check, when 
  mqtt_publish();

  if (!device.connected())
    return 0;
  return 1;
}

static void device_driver(void) {
// Rather optimistic main driver/statemachine as for now.

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
