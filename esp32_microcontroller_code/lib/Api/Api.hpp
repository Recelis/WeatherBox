#ifndef Api_H
#define Api_H

#include <ArduinoJson.h>
#include <HTTPClient.h>

class Api
{
private:
    HTTPClient http;
    bool isInit;

protected:
    char *url = nullptr;
    JsonDocument filters;
    JsonDocument data;
    void copyString(char *&target, const char *source);
    virtual void setupUrl() = 0;
    virtual void setupFilters() = 0;

public:
    Api();
    virtual ~Api();
    bool isSuccess = false;
    void init();
    void request();
};

#endif