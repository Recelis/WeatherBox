#ifndef LCDBoxCurrent_H
#define LCDBoxCurrent_H

class LCDBoxCurrent
{
private:
public:
    const char * location;
    float temp;
    float humidity;
    const char * weatherMain;
    LCDBoxCurrent();
    LCDBoxCurrent(const char *loc, float t, float h, const char *main);
    ~LCDBoxCurrent();
};

#endif
