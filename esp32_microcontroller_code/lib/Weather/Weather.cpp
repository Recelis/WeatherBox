#include "Weather.hpp"

Weather::Weather() : Api("Weather") {}

Weather::~Weather()
{
    delete weatherApiKey;
}

void Weather::configure(const char *apiKey, float lat, float lon)
{
    weatherApiKey = apiKey;
    latitude = lat;
    longitude = lon;
}

void Weather::setupUrl()
{
    String weatherURLString = "https://api.pirateweather.net/forecast/" + String(weatherApiKey) + "/" + String(latitude) + "," + String(longitude) + "?&units=ca&exclude=minutely,hourly,alerts";
    Serial.print("Weather url: ");
    Serial.println(weatherURLString);
    const char *weatherUrl = weatherURLString.c_str();
    copyString(url, weatherUrl);
}

void Weather::setupFilters()
{
    filters["currently"]["temperature"] = true;
    filters["currently"]["summary"] = true;
    filters["currently"]["humidity"] = true;

    filters["daily"]["data"][0]["summary"] = true;
    filters["daily"]["data"][0]["temperatureLow"] = true;
    filters["daily"]["data"][0]["temperatureHigh"] = true;
    filters["daily"]["data"][0]["humidity"] = true;
}

char *Weather::getData()
{
    size_t len = measureJson(data) + 1;
    Serial.print("length of weather data: ");
    Serial.println(len);
    char *result = new char[len];
    serializeJson(data, result, len);
    Serial.println("Weather data: ");
    Serial.println(result);
    return result;
}