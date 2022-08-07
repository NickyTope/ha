#include "arduino_secrets.h"
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <AceButton.h>
using namespace ace_button;

// Replace the next variables with your SSID/Password combination
char* ssid = SECRET_SSID;
char* password = SECRET_PASSWORD;

const char* mqtt_server = SECRET_MQTT_SERVER;
const char* mqtt_user = SECRET_MQTT_USER;
const char* mqtt_pass = SECRET_MQTT_PASS;

String mqtt_id = SECRET_MQTT_ID;
String mqtt_root = "dinky/button/";
String surl = mqtt_root + mqtt_id + (char *)("/status");
const char* mqtt_status = surl.c_str();
String turl = mqtt_root + mqtt_id + (char *)("/button1/state");
const char* mqtt_button1 = turl.c_str();
String turl2 = mqtt_root + mqtt_id + (char *)("/button2/state");
const char* mqtt_button2 = turl2.c_str();

const int BUT1_PIN = 4;
const int BUT2_PIN = 5;
AceButton but1(BUT1_PIN);
AceButton but2(BUT2_PIN);

WiFiClient espClient;
PubSubClient client(espClient);

char hexMAC[3 + 12 + 1] = "espxxxxxxxxxxxx"; // length of "esp" + 12 bytes for MAC + 1 byte '\0' terminator
uint8_t mac[6];

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
  
  client.publish(mqtt_status, "online");
  Serial.println("\nconnected!");
}

// Forward reference to prevent Arduino compiler becoming confused.
void handleEvent(AceButton*, uint8_t, uint8_t);

void setup() {
  Serial.begin(115200);
  Serial.println("Let's go!");

  // use pullup resistors
  pinMode(BUT1_PIN, INPUT_PULLUP);
  pinMode(BUT2_PIN, INPUT_PULLUP);

  ButtonConfig* buttonConfig = ButtonConfig::getSystemButtonConfig();
  buttonConfig->setEventHandler(handleEvent);
  buttonConfig->setFeature(ButtonConfig::kFeatureDoubleClick);
  buttonConfig->setFeature(
      ButtonConfig::kFeatureSuppressClickBeforeDoubleClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureSuppressAfterClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureSuppressAfterDoubleClick);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
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

unsigned long lastCount = 0;

void loop() {
  client.loop();
  but1.check();
  but2.check();
  if (!client.connected()) {
    connect();
    delay(10);
  }
}

void handleEvent(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  bool but1_action = button->getPin() == BUT1_PIN;
  
  switch (eventType) {
    case AceButton::kEventClicked:
      Serial.println('clicked');
      if (but1_action) {
        client.publish(mqtt_button1, "PRESS");
      } else {
        client.publish(mqtt_button2, "PRESS");
      }
      break;
    case AceButton::kEventDoubleClicked:
      Serial.println('double');
      if (but1_action) {
        client.publish(mqtt_button1, "DOUBLE");
      } else {
        client.publish(mqtt_button2, "DOUBLE");
      }
      break;
  }
}

char hex_digit(uint8_t aValue) {
  char values[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
  if (aValue < 16) {
    return values[aValue];
  }
  return 'x';
}
