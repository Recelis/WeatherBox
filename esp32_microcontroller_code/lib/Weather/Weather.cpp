#include "Weather.h"

Weather::Weather()
{
}

void Weather::setWeatherURL(String urlString)
{
    char *url = new char[urlString.length() + 1]; // set size of string block
    strcpy(url, urlString.c_str());
    weatherURL = url;
}

bool Weather::getWeather()
{
    // checks that 10 mins have passed before sending new request
    if (onStart | ((millis() - lastTime) > timerDelay))
    {
        callWeatherAPI();
        // send Serial data to Mega

        if (isNewData)
        {
            lastTime = millis(); // reset only if successful
            return true;         // sending new data only if have been set true by weather response
        }
    }
    return false; // sending old data
}

void Weather::callWeatherAPI()
{
    Serial.println("callWeatherAPI");
    http.begin(weatherURL);

    // Send HTTP GET request
    int httpResponseCode = http.GET();
    if (httpResponseCode == HTTP_CODE_OK)
    {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        // get length of document (is -1 when Server sends no Content-Length header)
        int len = http.getSize();
        if (len > 10000)
        {
            Serial.print("Error: Response is too long!");
            isNewData = false;
            return;
        }
        char *payload;
        Serial.print("length of response: ");
        Serial.println(len);
        // create buffer for read
        char buff[len] = {0};
        payload = buff;
        // get tcp stream
        WiFiClient *stream = http.getStreamPtr();

        // read all data from server
        while (http.connected() && (len > 0 || len == -1))
        {
            // get available data size
            size_t size = stream->available();

            if (size)
            {
                // read up to 128 byte
                int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));

                if (len > 0)
                {
                    len -= c;
                }
            }
            delay(1);
        }
        Serial.println(payload);
        Serial.println("");
        Serial.println("");
        Serial.println("");
        formatSevenDayForecast(payload);
        onStart = false; // let timer know to measure by time difference now.
        isNewData = true;
    }
    else
    {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
        isNewData = false; // is error then do another call.
    }
    // Free resources
    http.end();
}

void Weather::formatSevenDayForecast(char *payload)
{
    Serial.println("formatSevenDayForecast");
    // get an arduinojson of http payload
    int size = sizeof(payload);
    // Serial.println(payload);
    DynamicJsonDocument doc(1600);
    // apply filters
    StaticJsonDocument<500> filter;

    filter["currently"]["temperature"] = true;
    filter["currently"]["summary"] = true;
    filter["currently"]["humidity"] = true;

    filter["daily"]["data"][0]["summary"] = true;
    filter["daily"]["data"][0]["temperatureLow"] = true;
    filter["daily"]["data"][0]["temperatureHigh"] = true;
    filter["daily"]["data"][0]["humidity"] = true;

    deserializeJson(doc, payload, DeserializationOption::Filter(filter));
    memset(sevenDayForecast, 0, strlen(sevenDayForecast)); // reset sevenDayForecast
    serializeJson(doc, sevenDayForecast);                  // convert back to string
    Serial.println(sevenDayForecast);
    doc.clear();
}

char *Weather::getSevenDayForecast()
{
    return sevenDayForecast;
}

Weather::~Weather()
{
}