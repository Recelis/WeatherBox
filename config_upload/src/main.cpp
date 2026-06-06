#include <Arduino.h>
#include <Preferences.h>
#include "config.h"

// Keys must match Environment.hpp keyNames[] exactly (NVS max 15 chars)
const char* THING_NAME_KEY       = "THING_NAME";
const char* WIFI_SSID_KEY        = "WIFI_SSID";
const char* WIFI_PASSWORD_KEY    = "WIFI_PASSWORD";
const char* AWS_IOT_ENDPOINT_KEY = "AWS_IOT_ENDPNT";
const char* AWS_CERT_CA_KEY      = "AWS_CERT_CA";
const char* AWS_CERT_CRT_KEY     = "AWS_CERT_CRT";
const char* AWS_CERT_PRIVATE_KEY = "AWS_CERT_PRIVT";
const char* WEATHER_API_KEY_KEY  = "WEATHERAPI_KEY";

void setup() {
  Serial.begin(9600);
  Serial.println("Writing values into EEPROM");
  Preferences preferences;
  preferences.begin("ESP32Monitoring", false);
  preferences.putString(THING_NAME_KEY,       THING_NAME);
  // Only overwrite WiFi credentials if they are non-empty in config.h,
  // so a config_upload run for AWS certs doesn't wipe captive-portal credentials.
  if (strlen(WIFI_SSID) > 0)     preferences.putString(WIFI_SSID_KEY,     WIFI_SSID);
  if (strlen(WIFI_PASSWORD) > 0) preferences.putString(WIFI_PASSWORD_KEY, WIFI_PASSWORD);
  preferences.putString(AWS_IOT_ENDPOINT_KEY, AWS_IOT_ENDPOINT);
  preferences.putString(AWS_CERT_CA_KEY,      AWS_CERT_CA);
  preferences.putString(AWS_CERT_CRT_KEY,     AWS_CERT_CRT);
  preferences.putString(AWS_CERT_PRIVATE_KEY, AWS_CERT_PRIVATE);
  preferences.putString(WEATHER_API_KEY_KEY,  WEATHER_API_KEY);
  Serial.println("Finished writing values to EEPROM");

  Serial.println("Validating...");
  bool ok = true;
  ok &= preferences.getString(THING_NAME_KEY,       "").equals(THING_NAME);
  ok &= preferences.getString(WIFI_SSID_KEY,        "").equals(WIFI_SSID);
  ok &= preferences.getString(AWS_IOT_ENDPOINT_KEY, "").equals(AWS_IOT_ENDPOINT);
  Serial.println(ok ? "EEPROM values validated!" : "EEPROM validation failed.");
  preferences.end();
}

void loop() {}
