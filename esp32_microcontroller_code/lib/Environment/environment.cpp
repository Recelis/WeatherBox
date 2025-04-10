#include "Environment.hpp"
/*
    Wrapper class for Preferences
    to call environment values.
*/
Environment::Environment()
{
}

void Environment::begin(const char *environmentNamespace)
{
    preferences.begin(environmentNamespace, true);

    for (int i = 0; i < KEY_COUNT; ++i)
    {
        values[i] = retrieveFromPreference(keyNames[i]);
    }
    preferences.end();
}

String Environment::retrieveFromPreference(const char *key)
{
    return preferences.getString(key, "");
}

const char *Environment::get(Key key)
{
    return values[key].c_str();
}

Environment::~Environment()
{
}