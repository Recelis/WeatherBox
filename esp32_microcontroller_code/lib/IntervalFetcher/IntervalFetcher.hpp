#ifndef IntervalFetcher_HPP
#define IntervalFetcher_HPP

class IntervalFetcher
{
private:
    unsigned long intervalMs;
    unsigned long lastFetchTime;
    bool firstRun;

public:
    IntervalFetcher(unsigned long interval);
    bool shouldFetch();
    void setInterval(unsigned long newInterval);
    void resetTimer();
};

#endif