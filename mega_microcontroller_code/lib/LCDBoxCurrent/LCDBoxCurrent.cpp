#include "LCDBoxCurrent.h"

LCDBoxCurrent::LCDBoxCurrent()
{
    
}

LCDBoxCurrent::LCDBoxCurrent(const char * loc, float t, float h, const char * main)
{
    location = loc;
    temp = t;
    humidity = h;
    weatherMain = main;
}

LCDBoxCurrent::~LCDBoxCurrent()
{
}