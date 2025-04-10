#ifndef MegaCommunication_H
#define MegaCommunication_H

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