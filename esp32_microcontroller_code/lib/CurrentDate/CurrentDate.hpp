#ifndef CurrentDate_HPP
#define CurrentDate_HPP

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "Api.hpp"
class CurrentDate : public Api
{
private:
    float latitude;
    float longitude;

protected:
    void setupUrl() override;
    void setupFilters() override;

public:
    CurrentDate();
    ~CurrentDate();
    void configure(float lat, float lon);
    String getDayOfWeek();
    String getDate();
    String getDateTime();
};

#endif