#include "environment.h"
/*
    Wrapper class for Preferences
    to call environment values.
*/
Environment::Environment()
{
    THING_NAME_KEY = "THING_NAME";
    WIFI_SSID_KEY = "WIFI_SSID";
    WIFI_PASSWORD_KEY = "WIFI_PASSWORD";
    AWS_IOT_ENDPOINT_KEY = "AWS_IOT_ENDPNT";
    AWS_CERT_CA_KEY = "AWS_CERT_CA";
    AWS_CERT_CRT_KEY = "AWS_CERT_CRT";
    AWS_CERT_PRIVATE_KEY = "AWS_CERT_PRIVT";
    WEATHER_API_KEY = "WEATHERAPI_KEY";
}

void Environment::begin(const char *environmentNamespace)
{
    preferences.begin(environmentNamespace, true);
}

char *Environment::retrieveFromPreference(const char *key)
{
    const char *value = preferences.getString(key, "").c_str();
    Serial.println(value);
    return value;
}

Environment::~Environment()
{
}