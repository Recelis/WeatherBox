#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <ArduinoJson.h>
#include <Preferences.h>

class Environment
{
private:
    Preferences preferences;

public:
    Environment();
    ~Environment();
    const char* THING_NAME_KEY;
    const char* WIFI_SSID_KEY;
    const char* WIFI_PASSWORD_KEY;
    const char* AWS_IOT_ENDPOINT_KEY;
    const char* AWS_CERT_CA_KEY;
    const char* AWS_CERT_CRT_KEY;
    const char* AWS_CERT_PRIVATE_KEY;
    const char* WEATHER_API_KEY;

    void begin(const char* environmentNamespace);
    char* retrieveFromPreference(const char* key);
};

#endif