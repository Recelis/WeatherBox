#ifndef LOCATION_USING_IP_ADDRESS_H
#define LOCATION_USING_IP_ADDRESS_H

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