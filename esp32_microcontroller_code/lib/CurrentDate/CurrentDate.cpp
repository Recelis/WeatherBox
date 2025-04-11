#include "CurrentDate.hpp"
/*
    Calls TimeAPI to get dayOfWeek.
*/
CurrentDate::CurrentDate() : Api("CurrentDate")
{
}

void CurrentDate::configure(float lat, float lon)
{
    latitude = lat;
    longitude = lon;
}

void CurrentDate::setupUrl()
{
    String timeApiURLString = "https://www.timeapi.io/api/Time/current/coordinate?latitude=" + String(latitude) + "&longitude=" + String(longitude);
    Serial.print("current date url: ");
    Serial.println(timeApiURLString);
    const char *currentDateUrl = timeApiURLString.c_str();
    copyString(url, currentDateUrl);
}

void CurrentDate::setupFilters()
{
    filters["date"] = true;
    filters["dayOfWeek"] = true;
    filters["dateTime"] = true;
}

String CurrentDate::getDayOfWeek()
{
    return data["dayOfWeek"].as<String>();
}

String CurrentDate::getDate()
{
    return data["date"].as<String>();
}

String CurrentDate::getDateTime()
{
    return data["dateTime"].as<String>();
}

CurrentDate::~CurrentDate()
{
}
