#ifndef Api_H
#define Api_H

#include <ArduinoJson.h>
#include <HTTPClient.h>

class Api
{
private:
    HTTPClient http;
    char *url;
    char *data;

public:
    Api(const char *url);
    ~Api();
    void request(const char *filter = nullptr);
    void updateUrl(const char *url);
    virtual char *getData();
};

#endif