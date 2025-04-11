#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP

#include <ArduinoJson.h>
#include <Preferences.h>

class Environment
{

public:
    Environment();
    ~Environment();

    enum Key
    {
        THING_NAME,
        WIFI_SSID,
        WIFI_PASSWORD,
        AWS_IOT_ENDPOINT,
        AWS_CERT_CA,
        AWS_CERT_CRT,
        AWS_CERT_PRIVATE_KEY,
        WEATHER_API_KEY,
        KEY_COUNT // always keep this last
    };

    const char *keyNames[KEY_COUNT] = {
        "THING_NAME",
        "WIFI_SSID",
        "WIFI_PASSWORD",
        "AWS_IOT_ENDPNT",
        "AWS_CERT_CA",
        "AWS_CERT_CRT",
        "AWS_CERT_PRIVT",
        "WEATHERAPI_KEY"};

    void begin(const char *environmentNamespace);
    const char *get(Key key);

private:
    String values[KEY_COUNT];

    String retrieveFromPreference(const char *key);

    Preferences preferences;
};

#endif