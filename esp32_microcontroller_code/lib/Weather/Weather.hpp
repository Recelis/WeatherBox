#ifndef WEATHER_H
#define WEATHER_H

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "Api.hpp"
class Weather : public Api
{
private:
    const char *weatherApiKey;
    float latitude;
    float longitude;

protected:
    void setupUrl() override;
    void setupFilters() override;

public:
    Weather();
    ~Weather();
    void configure(const char *weatherApiKey, float latitude, float longitude);
    char *getData();
};

#endif