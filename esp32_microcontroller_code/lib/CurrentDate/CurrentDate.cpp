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

char *CurrentDate::getDayOfWeek()
{
    const char *day = data["dayOfWeek"];
    Serial.println(day);
    if (!day)
    {
        day = ""; // fallback if null
        isSuccess = false;
    }

    size_t len = strlen(day) + 1;
    Serial.print("day length: ");
    Serial.println(len);

    char *result = new char[len];
    strcpy(result, day);

    Serial.print("dayOfWeek: ");
    Serial.println(result);
    return result;
}

char *CurrentDate::getDate()
{
    const char *date = data["date"];
    Serial.println(date);
    if (!date)
    {
        date = ""; // fallback if null
        isSuccess = false;
    }

    size_t len = strlen(date) + 1;
    Serial.print("date length: ");
    Serial.println(len);

    char *result = new char[len];
    strcpy(result, date);

    Serial.print("date: ");
    Serial.println(result);
    return result;
}

CurrentDate::~CurrentDate()
{
}
