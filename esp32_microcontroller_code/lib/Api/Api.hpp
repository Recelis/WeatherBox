#ifndef Api_HPP
#define Api_HPP

#include <ArduinoJson.h>
#include <HTTPClient.h>

class Api
{
private:
    HTTPClient http;
    const char *apiName;
    bool isInit = false;

protected:
    char *url = nullptr;
    StaticJsonDocument<500> filters;
    DynamicJsonDocument data;
    void copyString(char *&target, const char *source);
    virtual void setupUrl() = 0;
    virtual void setupFilters() = 0;

public:
    Api(const char *name);
    virtual ~Api();
    bool isSuccess = false;
    void init();
    void request();
};

#endif