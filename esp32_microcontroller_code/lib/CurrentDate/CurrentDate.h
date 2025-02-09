#ifndef CurrentDate_H
#define CurrentDate_H

#include <ArduinoJson.h>
#include <HTTPClient.h>

class CurrentDate
{
private:
    HTTPClient http;
    char *dayOfWeek;

public:
    CurrentDate();
    ~CurrentDate();
    void requestDayOfWeek(float latitude, float longitude);
    char *getDayOfWeek();
};

#endif