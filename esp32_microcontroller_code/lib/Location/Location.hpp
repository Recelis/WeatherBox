#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "Api.hpp"

class Location : public Api
{
private:
    float latitude;
    float longitude;

protected:
    void setupUrl() override;
    void setupFilters() override;

public:
    Location();
    ~Location();
    String getCity();
    float getLatitude();
    float getLongitude();
};

#endif