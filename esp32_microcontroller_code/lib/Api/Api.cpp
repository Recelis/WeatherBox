#include "Api.h"

// Api initialised only on use.
Api::Api(const char *url)
{
    http.begin(url);
}

Api::~Api()
{
    http.end();
    delete url;
    delete data;
}

void Api::request(const char *url)
{
    // Send HTTP GET request
    int httpResponseCode = http.GET();

    if (httpResponseCode == HTTP_CODE_OK)
    {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        // deal with dynamic load
        DynamicJsonDocument doc(5024);
        deserializeJson(doc, payload);

        dayOfWeek = strdup(doc["dayOfWeek"]); // doc["city"] get's overwritten by later uses of DynamicJSON therefore copy to city
        doc.clear();
    }
    else
    {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();
}

char *Api::getDayOfWeek()
{
    return dayOfWeek;
}

Api::~CurrentDate()
{
}
