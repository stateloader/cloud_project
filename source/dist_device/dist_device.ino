#include "configs.h"

// I've put all functions in this .ini-file because I don't really know how to import logic
// from my own custom made modules without being yelled at by either the Arduino-IDE, or the
// compiler. Frankly, usually both.

// This flash runs in yolo-mode with no serious error-handling in place at all yet. I've
// utilized stdint.h mainly because of my kink for the uint8_t type.

time_t datetime;

uint16_t sensor_value = 0;               // Current sample (published in 1 sec intervals).
uint8_t update_event = 0;
uint8_t vessel_state = VESSEL_GRN;       // Current vessel_state.

uint8_t threshold_YEL = 10;              // Stores desired threshold for yellow.
uint8_t threshold_RED = 20;              // Stores desired threshold for red.

uint8_t device_state = DEVICE_CONNECT;

static void mqtt_publish(void) {

// Publish 'vessel_state' and 'sensor_value' to the cloud among some friends. Messages will
// only be sent (and stored in cloud) if the 'update_event' attribute will be sent. Storage
// Storage is precious.

  delay(1000);
  char mqtt_buff[512];
  StaticJsonDocument<200> doc;

  doc["device_id"] = DEVICE_ID;
  doc["vessel_state"] = vessel_state;
  doc["sensor_value"] = sensor_value;
  doc["update_event"] = update_event;

  serializeJson(doc, mqtt_buff);
  device.publish(TOPIC_PUB, mqtt_buff);

  update_event = 0;
}

static void mqtt_receive(char *topic, byte *payload, unsigned int length) {

// As for now, the only thing possible to receive from the cloud is threshold values. These are
// "commanded" from the cloud by an assigned employee in my imaginary Waste management company.

  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);

  threshold_YEL = doc["threshold_YEL"];
  threshold_RED = doc["threshold_RED"];
}

static uint8_t connect_lan(void) {

// Establish LAN connection.

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  Serial.println("Connecting to LAN...");
  while (WiFi.status() != WL_CONNECTED)

  return 1;
}

static uint8_t connect_ntp(void) {

// Time syncing with NTP.

  Serial.println("LAN Connected!");
  Serial.println("Syncing clock..");
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

  Serial.println("Connecting to AWS...");
  while (!device.connect(DEVICE_ID))

  device.subscribe(TOPIC_SUB);

  return 1;
}

static uint8_t connect_routine(void) {

// "Wrapper" for connectivity-logic. 'state-values' in place for error-handling down the road.

  uint8_t states = 0;

  states += connect_lan();
  states += connect_ntp();
  states += connect_aws();

  if (states == 3)
    return 1;
  return 0;
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

  if (sensor_value < threshold_YEL) {
    vessel_state = VESSEL_GRN;
  } else if (sensor_value >= threshold_RED) {
    vessel_state = VESSEL_RED;
  } else {
    vessel_state = VESSEL_YEL;
  }
}

static void update_eval(void) {

// Evaluates/checks if last 'sensor_value' has reached/passed a threshold. If so, this might
// just be temporarly when people throwing stuff or the sensor has catched a fly.

// I've implemented a 30 sec 'event_buffer' for this reason. If 'last_vessel_state' still
// differ from 'vessel_state' after this period, a message with the 'update' attribute set
// will be published to the cloud.

  static uint64_t event_time_stamp   = 0; // Stores "millis()" for the event.
  static uint8_t  prev_vessel_state  = 0;
  
  if (event_time_stamp) {

    if (millis() - event_time_stamp < EVENT_TIMER)
      return;

    update_event = (prev_vessel_state != vessel_state) ? 1 : 0;
    prev_vessel_state = vessel_state;    
    event_time_stamp = 0;

  } else if (vessel_state != prev_vessel_state) {
      event_time_stamp = millis();

  } else {
    prev_vessel_state = vessel_state;
  }
}

uint8_t ongoing_routine(void) {

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
