#include "Api.hpp"

// Api initialised only on use.
Api::Api(const char *name) : data(1600)
{
    apiName = name;
}

Api::~Api()
{
    delete[] url;
}

void Api::init()
{
    // new line
    Serial.print(apiName);
    isInit = true; // this allows requests to continue being made.

    setupUrl();
    setupFilters();
}

void Api::request()
{
    // check that all url has been initialised
    if (!isInit)
    {
        Serial.print(apiName);
        Serial.println(" has not been initialised yet!");
        return;
    }
    // clear data before requesting again
    data.clear();

    http.begin(url);
    http.setTimeout(10000);
    // stop reuse of http client https://github.com/espressif/arduino-esp32/issues/3387#issuecomment-756815055
    // this is an issue with the timeapi endpoint which would prevent headers from being sent a second time.
    http.setReuse(false);

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
        switch (httpResponseCode)
        {
        case HTTPC_ERROR_CONNECTION_LOST:
            Serial.println("Connection lost — maybe TLS/SSL or server closed the connection.");
            break;
        case HTTPC_ERROR_SEND_HEADER_FAILED:
            Serial.println("Send header failed.");
            break;
        case HTTPC_ERROR_READ_TIMEOUT:
            Serial.println("Read timeout.");
            break;
        default:
            Serial.println("Unknown error — check SSL certs, memory, and endpoint.");
        }
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