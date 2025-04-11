#include "IntervalFetcher.hpp"
#include <Arduino.h>

IntervalFetcher::IntervalFetcher(unsigned long interval) : intervalMs(interval), lastFetchTime(0), firstRun(true) {}

bool IntervalFetcher::shouldFetch()
{
    unsigned long now = millis();
    if (firstRun || now - lastFetchTime >= intervalMs)
    {
        lastFetchTime = now;
        firstRun = false;
        return true;
    }
    return false;
}

void IntervalFetcher::setInterval(unsigned long newInterval)
{
    intervalMs = newInterval;
}

void IntervalFetcher::resetTimer()
{
    lastFetchTime = millis() - intervalMs;
    firstRun = true;
}