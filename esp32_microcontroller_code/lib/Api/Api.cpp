#include "Api.hpp"

// Api initialised only on use.
Api::Api(const char *name) : data(1600)
{
    apiName = name;
}

Api::~Api()
{
    http.end();
    delete[] url;
}

void Api::init()
{
    if (!isInit)
    {
        setupUrl();
        setupFilters();
        http.begin(url);
        isInit = true;
    }
}

void Api::request()
{
    // clear data before requesting again
    data.clear();

    if (!isInit)
    {
        Serial.print(apiName);
        Serial.println(" has not been initialised!");
        return;
    }
    Serial.println("");
    Serial.print("Starting request of ");
    Serial.println(apiName);
    isSuccess = false;
    // Send HTTP GET request
    int httpResponseCode = http.GET();

    if (httpResponseCode == HTTP_CODE_OK)
    {
        Serial.print(apiName);
        Serial.print(" HTTP Response code: ");
        Serial.println(httpResponseCode);
        // deserialize and automatically filter http response as a string
        DeserializationError error = deserializeJson(data, http.getString(), DeserializationOption::Filter(filters));
        if (error)
        {
            Serial.print(F("Deserialization failed: "));
            Serial.println(error.f_str());
        }
        else
        {
            isSuccess = true;
        }
        int contentLength = http.getSize();
        Serial.print("Content length: ");
        Serial.println(contentLength);
    }
    else
    {
        Serial.print("HTTP Error code: ");
        Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();
}

void Api::copyString(char *&target, const char *source)
{
    size_t len = strlen(source) + 1;
    target = new char[len];
    strcpy(target, source);
}