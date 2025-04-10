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

String Location::getCity()
{
    return data["city"].as<String>();
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