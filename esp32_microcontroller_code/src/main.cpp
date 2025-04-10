#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include <Preferences.h>

#include <Location.hpp>
#include <Weather.hpp>
#include <MegaCommunication.h>
#include <Environment.hpp>
#include <CurrentDate.hpp>

SET_LOOP_TASK_STACK_SIZE(16 * 1024);

/*
  ESP32 Weather built on PlatformIO.

    Functionality
      Config Mode
        sets EEPROM using Preferences.h for wifi ssid and password

      Start
        on start, will get location of Weather Viewer using IP address

      Every half hour
        calls open-weather endpoint
        extracts 7 days weather data using ArduinoJSON and builds an object containing resulting data
        serialises results and sends to Arduino Mega.

*/

// initialise Location object
Location location;

// initialise weather object
Weather weather;

// initialise communication object
MegaCommunication megaCommunication;

// initialise environment object
Environment environment;

// initialise day of week object
CurrentDate currentDate;

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

#define MICROSECONDS_IN_SECOND 1000
#define SECONDS_IN_MINUTE 60
#define LOOP_MICROSECONDS 100
#define PUBLISH_MICROSECONDS 2 * SECONDS_IN_MINUTE *MICROSECONDS_IN_SECOND

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);
Environment env = Environment();

void messageHandler(String &topic, String &payload)
{
  Serial.println("incoming: " + topic + " - " + payload);
}

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

void connectAWS()
{
  // Get variables from EEPROM (Preference.h)
  const char *THING_NAME = env.get(Environment::THING_NAME);
  const char *AWS_CERT_CA = env.get(Environment::AWS_CERT_CA);
  const char *AWS_CERT_CRT = env.get(Environment::AWS_CERT_CRT);
  const char *AWS_CERT_PRIVATE = env.get(Environment::AWS_CERT_PRIVATE_KEY);
  const char *AWS_IOT_ENDPOINT = env.get(Environment::AWS_IOT_ENDPOINT);

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Create a message handler
  client.onMessage(messageHandler);

  Serial.println("Connecting to AWS IOT");

  while (!client.connect(THING_NAME))
  {
    Serial.print(".");
    delay(100);
  }

  if (!client.connected())
  {
    Serial.println("AWS IoT Timeout!");
    return;
  }
  Serial.println("Connected to AWS IOT!");

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
}

void getWeatherData()
{
  location.init();
  location.request();
  if (!location.isSuccess)
  {
    return;
  }

  currentDate.configure(location.getLatitude(), location.getLongitude());
  currentDate.init();
  currentDate.request();

  if (!currentDate.isSuccess)
  {
    return;
  }
  weather.configure(env.get(Environment::WEATHER_API_KEY), location.getLatitude(), location.getLongitude());
  weather.init();
  weather.request();
}

void setup()
{
  Serial.begin(9600);
  Serial.println("Starting Program");
  env.begin("ESP32Monitoring");
  connectWifi();
  // connectAWS();
  // publishMessage();
}

void loop()
{
  // get weather and display on screen
  getWeatherData();

  // if newWeatherData then send to Mega
  if (location.isSuccess && currentDate.isSuccess && weather.isSuccess)
  {
    Serial.println("Sending new Data:");
    char *sevenDayForecast = weather.getData();
    String dayOfWeek = currentDate.getDayOfWeek();
    String city = location.getCity();
    megaCommunication.sendData(sevenDayForecast, city, dayOfWeek);
    delete[] sevenDayForecast;
  }
  else
  {
    if (!location.isSuccess)
      Serial.print("Location ");
    if (!currentDate.isSuccess)
      Serial.print("CurrentDate ");
    if (!weather.isSuccess)
      Serial.print("Weather ");
    Serial.println("Failure when requesting weather data");
  }
  Serial.print("Free heap: ");
  Serial.println(ESP.getFreeHeap());
  delay(3600000);
}