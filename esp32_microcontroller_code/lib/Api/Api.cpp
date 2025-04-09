#include "Api.hpp"

// Api initialised only on use.
Api::Api()
{
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
    isSuccess = false;
    // Send HTTP GET request
    int httpResponseCode = http.GET();

    if (httpResponseCode == HTTP_CODE_OK)
    {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        // deserialize and automatically filter http response as a stream
        deserializeJson(data, http.getStream(), DeserializationOption::Filter(filters));
        isSuccess = true;
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