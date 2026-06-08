#ifndef LCD_H
#define LCD_H

#include <Adafruit_I2CDevice.h>
#include <SPI.h>
#include "Adafruit_GFX.h"
#include <ArduinoJson.h>
#include <MCUFRIEND_kbv.h>
#include <stdio.h>

// 16-bit colour values (5-6-5 RGB)
#define BLACK    0x0000
#define WHITE    0xFFFF
#define YELLOW   0xFFE0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define BLUE     0x001F
#define GREEN    0x07E0
#define ORANGE   0xFD20
#define DARKGREY 0x4208

// Screen layout (landscape 480×320)
#define SCREEN_W  480
#define SCREEN_H  320
#define HEADER_H   20
#define ROW_H      60
#define MAX_ROWS    5
#define ROW_PAD     4

// Idle timeout: redraw idle screen if no data for this many ms
#define IDLE_TIMEOUT_MS 300000UL  // 5 minutes

class LCD
{
private:
    MCUFRIEND_kbv tft;
    unsigned long lastDataMs = 0;
    bool _idle = false;
    bool _pollSent = false;

    void drawHeader();
    void drawRow(int rowIndex, const char *source, const char *sender,
                 const char *channel, const char *preview,
                 const char *timeStr, uint8_t priority);
    uint16_t sourceColor(const char *source);
    void truncate(const char *src, char *dst, uint8_t maxLen);

public:
    LCD();
    ~LCD();
    void startScreen();
    void refreshScreen();
    void drawScreen(char *receivedChars);
    // Returns true once when idle timeout first fires — caller should send a poll request.
    bool checkIdle();
};

#endif
