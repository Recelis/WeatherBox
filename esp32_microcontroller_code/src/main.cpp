#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include <Preferences.h>

#include <MegaCommunication.h>
#include <Environment.hpp>
#include <IntervalFetcher.hpp>

SET_LOOP_TASK_STACK_SIZE(16 * 1024);

// MQTT topic the Node.js server publishes notification batches to
#define NOTIFICATIONS_SUBSCRIBE_TOPIC "weatherbox/notifications"

// Heartbeat to AWS IoT every 10 minutes to keep the connection alive
#define HEARTBEAT_INTERVAL_MS 600000

#define LOOP_DELAY_MS 100

WiFiClientSecure net = WiFiClientSecure();

// Increase MQTT buffer to handle up to 8 notifications × ~200 bytes each
MQTTClient client = MQTTClient(6000);

Environment env = Environment();
MegaCommunication megaCommunication;
IntervalFetcher heartbeatTimer = IntervalFetcher(HEARTBEAT_INTERVAL_MS);

// ── MQTT message handler ──────────────────────────────────────────────────────

void messageHandler(String &topic, String &payload)
{
  if (topic != NOTIFICATIONS_SUBSCRIBE_TOPIC) return;

  // Forward the raw JSON batch directly to the Mega over Serial.
  // The Mega's Communication library buffers until '\n' and then parses.
  megaCommunication.sendRaw(payload.c_str());
}

// ── WiFi ──────────────────────────────────────────────────────────────────────

void connectWifi()
{
  const char *WIFI_SSID = env.get(Environment::WIFI_SSID);
  const char *WIFI_PASSWORD = env.get(Environment::WIFI_PASSWORD);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Wi-Fi Connected!");
}

// ── AWS IoT MQTT ──────────────────────────────────────────────────────────────

void connectAWS()
{
  const char *THING_NAME     = env.get(Environment::THING_NAME);
  const char *AWS_CERT_CA    = env.get(Environment::AWS_CERT_CA);
  const char *AWS_CERT_CRT   = env.get(Environment::AWS_CERT_CRT);
  const char *AWS_CERT_PRIV  = env.get(Environment::AWS_CERT_PRIVATE_KEY);
  const char *AWS_ENDPOINT   = env.get(Environment::AWS_IOT_ENDPOINT);

  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIV);

  client.begin(AWS_ENDPOINT, 8883, net);
  client.onMessage(messageHandler);

  Serial.println("Connecting to AWS IoT");
  while (!client.connect(THING_NAME))
  {
    Serial.print(".");
    delay(100);
  }

  if (!client.connected())
  {
    Serial.println("AWS IoT connection timed out");
    return;
  }

  client.subscribe(NOTIFICATIONS_SUBSCRIBE_TOPIC);
  Serial.println("AWS IoT connected — subscribed to " NOTIFICATIONS_SUBSCRIBE_TOPIC);
}

// ── Setup & loop ──────────────────────────────────────────────────────────────

void setup()
{
  Serial.begin(9600);
  Serial.println("WeatherBox starting");
  env.begin("ESP32Monitoring");
  connectWifi();
  connectAWS();
}

void loop()
{
  // Process incoming MQTT messages
  client.loop();

  // Reconnect if connection dropped
  if (!client.connected())
  {
    Serial.println("MQTT disconnected — reconnecting");
    connectAWS();
  }

  // Heartbeat publish every 10 minutes to keep the AWS IoT session alive
  if (heartbeatTimer.shouldFetch())
  {
    client.publish("weatherbox/heartbeat", "{\"status\":\"alive\"}");
    Serial.print("Free heap: ");
    Serial.println(ESP.getFreeHeap());
  }

  delay(LOOP_DELAY_MS);
}
