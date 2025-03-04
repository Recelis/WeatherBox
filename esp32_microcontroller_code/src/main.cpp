#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include <Preferences.h>
#include <DHT.h>
#include "environment.h"

#include <Location.h>
#include <Weather.h>
#include <MegaCommunication.h>
#include <Environment.h>
#include <CurrentDate.h>

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

const char *locationURL = "https://ipapi.co/json/";

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

#define DHTPIN 23
#define DHTTYPE DHT22

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);
Environment env = Environment();
DHT dht(DHTPIN, DHTTYPE);

void messageHandler(String &topic, String &payload)
{
  Serial.println("incoming: " + topic + " - " + payload);

  //  StaticJsonDocument<200> doc;
  //  deserializeJson(doc, payload);
  //  const char* message = doc["message"];
}

void connectWifi()
{
  const char *WIFI_SSID = env.retrieveFromPreference(env.WIFI_SSID_KEY);
  const char *WIFI_PASSWORD = env.retrieveFromPreference(env.WIFI_PASSWORD_KEY);

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
  const char *THING_NAME = env.retrieveFromPreference(env.THING_NAME_KEY);
  const char *AWS_CERT_CA = env.retrieveFromPreference(env.AWS_CERT_CA_KEY);
  const char *AWS_CERT_CRT = env.retrieveFromPreference(env.AWS_CERT_CRT_KEY);
  const char *AWS_CERT_PRIVATE = env.retrieveFromPreference(env.AWS_CERT_PRIVATE_KEY);
  const char *AWS_IOT_ENDPOINT = env.retrieveFromPreference(env.AWS_IOT_ENDPOINT_KEY);

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

void publishMessage()
{
  float temp = dht.readTemperature();
  Serial.print("Temperature: ");
  Serial.println(temp);
  if (isnan(temp))
  {
    Serial.println("Failed to read temperature from sensor");
    return;
  }

  const char *THING_NAME = env.retrieveFromPreference(env.THING_NAME_KEY);

  StaticJsonDocument<200> doc;
  doc["time"] = millis();
  doc["temp"] = temp;
  doc["monitorName"] = THING_NAME;
  char jsonBuffer[512];

  serializeJson(doc, jsonBuffer); // print to client

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
  Serial.print("publish message: ");
  Serial.print(jsonBuffer);
  Serial.print(" ");
  Serial.println(AWS_IOT_PUBLISH_TOPIC);
}

int loopIncrement = 0;

void getWeatherData()
{
  location.getLocation(locationURL);
  const char *WEATHER_API_KEY = env.retrieveFromPreference(env.WEATHER_API_KEY);
  String weatherURLString = "https://api.pirateweather.net/forecast/" + String(WEATHER_API_KEY) + "/" + String(location.getLatitude()) + "," + String(location.getLongitude()) + "?&units=ca&exclude=minutely,hourly,alerts";
  Serial.println(weatherURLString);
  weather.setWeatherURL(weatherURLString);
  currentDate.requestDayOfWeek(location.getLatitude(), location.getLongitude());
}

void setup()
{
  Serial.begin(9600);
  Serial.println("Starting Program");
  dht.begin();
  env.begin("ESP32Monitoring");
  connectWifi();
  getWeatherData();
  // connectAWS();
  // publishMessage();
}

void loop()
{
  // Don't publish message
  // if (!client.connected()) {
  //   connectAWS();
  // }

  // client.loop();
  // delay(LOOP_MICROSECONDS);
  // if (loopIncrement > (PUBLISH_MICROSECONDS / LOOP_MICROSECONDS)) {
  //   publishMessage();
  //   loopIncrement = 0;
  // };
  // loopIncrement++;

  // get weather and display on screen
  bool isNewData = weather.getWeather();
  // if newWeatherData then send to Mega
  if (isNewData)
  {
    Serial.println("Sending new Data:");
    char *sevenDayForecast = weather.getSevenDayForecast();
    megaCommunication.sendData(sevenDayForecast, location.getCity(), currentDate.getDayOfWeek());
  }
}