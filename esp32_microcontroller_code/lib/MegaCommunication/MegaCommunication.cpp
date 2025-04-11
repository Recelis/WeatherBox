#include "MegaCommunication.h"

// Serial1 to Mega values
#define RXD 16
#define TXD 17

MegaCommunication::MegaCommunication()
{
  // Set up Mega Serial
  Serial1.begin(9600, SERIAL_8N1, RXD, TXD);
}

void MegaCommunication::sendData(char *sevenDayForecast, String location, String dayOfWeek)
{
  // combines the location city data with weather data
  // send to Mega
  JsonDocument doc;
  deserializeJson(doc, sevenDayForecast);
  doc["location"] = location;
  doc["dayOfWeek"] = dayOfWeek;
  char output[1500];
  serializeJson(doc, output);
  Serial1.println(output);
  Serial.println("Sending data: ");
  Serial.println(output);
}

MegaCommunication::~MegaCommunication()
{
}