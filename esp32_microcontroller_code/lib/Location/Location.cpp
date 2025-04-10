#include "Location.hpp"

Location::Location() : Api("Location")
{
}

void Location::setupUrl()
{
    const char *locationUrl = "https://ipapi.co/json/";
    Serial.print("location url: ");
    Serial.println(locationUrl);
    copyString(url, locationUrl);
}

void Location::setupFilters()
{
    filters["city"] = true;
    filters["latitude"] = true;
    filters["longitude"] = true;
}

char *Location::getCity()
{
    const char *city = data["city"];
    Serial.println(city);
    if (!city)
    {
        city = ""; // fallback if null
        isSuccess = false;
    }

    size_t len = strlen(city) + 1;
    Serial.print("city length: ");
    Serial.println(len);

    char *result = new char[len];
    strcpy(result, city);

    Serial.print("city: ");
    Serial.println(result);
    return result;
}

float Location::getLatitude()
{
    float latitude = data["latitude"];
    Serial.print("latitude: ");
    Serial.println(latitude);
    if (!latitude)
    {
        latitude = 0.0; // fallback if null
        isSuccess = false;
    }

    return latitude;
}

float Location::getLongitude()
{
    float longitude = data["longitude"];
    Serial.print("longitude: ");
    Serial.println(longitude);
    if (!longitude)
    {
        longitude = 0.0; // fallback if null
        isSuccess = false;
    }

    return longitude;
}

Location::~Location()
{
}