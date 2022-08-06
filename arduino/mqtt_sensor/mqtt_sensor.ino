#include "arduino_secrets.h"

#include <PubSubClient.h>
// Include the correct WiFi header file for the board we're running on
// This code will work for ESP8266, ESP32 and Arduino MKRWIFI1010
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#elif defined(ARDUINO_SAMD_MKRWIFI1010)
#include <WiFiNINA.h>
#elif defined(ARDUINO_SAMD_MKR1000)
#include <WiFi101.h>
#else
#error "Chosen board not implemented.  WiFi won't work yet"
#endif

// Replace the next variables with your SSID/Password combination
char* ssid = SECRET_SSID;
char* password = SECRET_PASSWORD;

const char* mqtt_server = SECRET_MQTT_SERVER;
const char* mqtt_user = SECRET_MQTT_USER;
const char* mqtt_pass = SECRET_MQTT_PASS;

// Expects a PIR sensor connected to a digital interrupt pin
const int kSensorPin = 14;

WiFiClient espClient;
PubSubClient client(espClient);

char hexMAC[3 + 12 + 1] = "espxxxxxxxxxxxx"; // length of "esp" + 12 bytes for MAC + 1 byte '\0' terminator
uint8_t mac[6];

char msg[50];
int value = 0;
unsigned long lastMillis = 0;

void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("\nconnecting...");
  while (!client.connect(hexMAC, mqtt_user, mqtt_pass)) {
    Serial.print(".");
    delay(1000);
  }

  client.publish("dinky/status", "online");
  client.publish("homeassistant/binary_sensor/dinky/config", "{\"name\": \"dinky\", \"unique_id\": \"dinkysensor\", \"device_class\": \"motion\", \"state_topic\": \"dinky/motion\"}");

  Serial.println("\nconnected!");
}


void setup() {
  Serial.begin(115200);

  Serial.println("Let's go!");

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  pinMode(kSensorPin, INPUT);
}


void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  WiFi.macAddress(mac);

  for (int i = 0; i < 6; i++)
  {
    hexMAC[3 + i * 2] = hex_digit(mac[i] >> 4);
    hexMAC[3 + i * 2 + 1] = hex_digit(mac[i] & 0x0f);
  }
  Serial.println(hexMAC);
}

void reconnect() {
  // Loop until we're reconnected
  client.loop();
  delay(10);

  if (!client.connected()) {
    connect();
  }
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(hexMAC, mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      // Subscribe
      //client.subscribe("dinky/test");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// store the on/off state so we only send a change
bool gMovementState = false;

void loop() {

  client.loop();
  delay(10);  // <- fixes some issues with WiFi stability

  if (!client.connected()) {
    connect();
  }

  if (digitalRead(kSensorPin))
  {
    // Movement detected
    if (!gMovementState) {
      Serial.println("detected");
      // Rising edge, send a message
      client.publish("dinky/motion", "ON");
      gMovementState = true;
    }
  }
  else
  {
    if (gMovementState) {
      Serial.println("cleared");
      // Falling edge
      client.publish("dinky/motion", "OFF");
      gMovementState = false;
    }
  }
  delay(500);
}

char hex_digit(uint8_t aValue) {
  char values[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
  if (aValue < 16) {
    return values[aValue];
  }
  return 'x';
}
