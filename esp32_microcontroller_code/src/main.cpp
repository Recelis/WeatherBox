#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include <Preferences.h>

#include <MegaCommunication.h>
#include <Environment.hpp>
#include <IntervalFetcher.hpp>
#include <WiFiProvisioner.h>

SET_LOOP_TASK_STACK_SIZE(16 * 1024);

#define NOTIFICATIONS_SUBSCRIBE_TOPIC "weatherbox/notifications"
#define WIFI_CONNECT_TIMEOUT_MS       10000
#define HEARTBEAT_INTERVAL_MS         600000
#define LOOP_DELAY_MS                 100

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client    = MQTTClient(6000);
Environment env      = Environment();
MegaCommunication megaCommunication;
IntervalFetcher heartbeatTimer = IntervalFetcher(HEARTBEAT_INTERVAL_MS);
WiFiProvisioner provisioner;

// ── MQTT message handler ──────────────────────────────────────────────────────

void messageHandler(String &topic, String &payload)
{
  if (topic != NOTIFICATIONS_SUBSCRIBE_TOPIC) return;
  megaCommunication.sendRaw(payload.c_str());
}

// ── WiFi — returns true on success, false on timeout ─────────────────────────

bool connectWifi()
{
  const char *ssid     = env.get(Environment::WIFI_SSID);
  const char *password = env.get(Environment::WIFI_PASSWORD);

  if (!ssid || strlen(ssid) == 0)
  {
    Serial.println("No WiFi credentials stored");
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    if (millis() - start > WIFI_CONNECT_TIMEOUT_MS)
    {
      Serial.println("\nWiFi connection timed out");
      return false;
    }
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected: " + WiFi.localIP().toString());
  return true;
}

// ── AWS IoT MQTT ──────────────────────────────────────────────────────────────

bool awsCredentialsPresent()
{
  const char *name     = env.get(Environment::THING_NAME);
  const char *endpoint = env.get(Environment::AWS_IOT_ENDPOINT);
  return name && strlen(name) > 0 && endpoint && strlen(endpoint) > 0;
}

void connectAWS()
{
  if (!awsCredentialsPresent())
  {
    Serial.println("AWS IoT credentials not configured — skipping MQTT. Flash config_upload to enable.");
    return;
  }

  const char *THING_NAME    = env.get(Environment::THING_NAME);
  const char *AWS_CERT_CA   = env.get(Environment::AWS_CERT_CA);
  const char *AWS_CERT_CRT  = env.get(Environment::AWS_CERT_CRT);
  const char *AWS_CERT_PRIV = env.get(Environment::AWS_CERT_PRIVATE_KEY);
  const char *AWS_ENDPOINT  = env.get(Environment::AWS_IOT_ENDPOINT);

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

  if (!connectWifi())
  {
    // Credentials missing or wrong — launch captive portal so the user can
    // enter new credentials from their phone. Reboots automatically on save.
    provisioner.begin();
  }

  connectAWS();
}

void loop()
{
  client.loop();

  if (!client.connected() && awsCredentialsPresent())
  {
    Serial.println("MQTT disconnected — reconnecting");
    connectAWS();
  }

  if (heartbeatTimer.shouldFetch())
  {
    client.publish("weatherbox/heartbeat", "{\"status\":\"alive\"}");
    Serial.print("Free heap: ");
    Serial.println(ESP.getFreeHeap());
  }

  delay(LOOP_DELAY_MS);
}
