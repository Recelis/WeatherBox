#ifndef MegaCommunication_HPP
#define MegaCommunication_HPP

#include <ArduinoJson.h>

class MegaCommunication
{
private:
public:
    MegaCommunication();
    ~MegaCommunication();
    void sendData(char *sevenDayForecast, String city, String dayOfWeek);
};

#endif