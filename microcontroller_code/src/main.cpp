#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include <Preferences.h>
#include "environment.h"

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);
Environment environment = Environment(); 

void messageHandler(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

//  StaticJsonDocument<200> doc;
//  deserializeJson(doc, payload);
//  const char* message = doc["message"];
}

void connectAWS()
{
  // Get variables from EEPROM (Preference.h)
  const char* THING_NAME = environment.retrieveFromPreference(environment.THING_NAME_KEY);
  const char* WIFI_SSID = environment.retrieveFromPreference(environment.WIFI_SSID_KEY);
  const char* WIFI_PASSWORD = environment.retrieveFromPreference(environment.WIFI_PASSWORD_KEY);
  const char* AWS_CERT_CA = environment.retrieveFromPreference(environment.AWS_CERT_CA_KEY);
  const char* AWS_CERT_CRT = environment.retrieveFromPreference(environment.AWS_CERT_CRT_KEY);
  const char* AWS_CERT_PRIVATE = environment.retrieveFromPreference(environment.AWS_CERT_PRIVATE_KEY);
  const char* AWS_IOT_ENDPOINT = environment.retrieveFromPreference(environment.AWS_IOT_ENDPOINT_KEY);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected!");
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

//   // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

//   // Create a message handler
  // client.onMessage(messageHandler);

  Serial.println("Connecting to AWS IOT");

  while (!client.connect(THING_NAME)) {
    Serial.print(".");
    delay(100);
  }

  if(!client.connected()){
    Serial.println("AWS IoT Timeout!");
    return;
  }
  Serial.println("Connected to AWS IOT!");

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

//   Serial.println("AWS IoT Connected!");
}

// void publishMessage()
// {
//   StaticJsonDocument<200> doc;
//   doc["time"] = millis();
//   doc["sensor_a0"] = analogRead(0);
//   char jsonBuffer[512];
//   serializeJson(doc, jsonBuffer); // print to client

//   client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
// }

void setup() {
  Serial.begin(9600);
  environment.begin("ESP32Monitoring");
  connectAWS();
}

void loop() {
  // publishMessage();
  client.loop();
  delay(1000);
  // Serial.println("testing connection");
}