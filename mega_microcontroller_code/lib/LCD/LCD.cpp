#include "LCD.h"

LCD::LCD() {}

void LCD::startScreen()
{
    uint16_t ID = tft.readID();
    if (ID == 0xD3D3) ID = 0x9481; // write-only shield fallback
    tft.begin(ID);
}

void LCD::refreshScreen()
{
    tft.setRotation(1); // landscape: 480×320
    tft.fillScreen(BLACK);
}

// ── Public entry point ────────────────────────────────────────────────────────

void LCD::drawScreen(char *receivedChars)
{
    // Parse the notification batch: {"notifications":[...]}
    // StaticJsonDocument keeps allocations on the stack (predictable on Mega).
    // 3000 bytes handles up to ~5 notifications comfortably within 8KB SRAM.
    StaticJsonDocument<3000> doc;
    DeserializationError err = deserializeJson(doc, receivedChars);

    if (err)
    {
        Serial.print("JSON parse error: ");
        Serial.println(err.c_str());
        return;
    }

    JsonArray notifications = doc["notifications"].as<JsonArray>();
    if (notifications.isNull())
    {
        Serial.println("No notifications array in payload");
        return;
    }

    refreshScreen();
    drawHeader();

    uint8_t row = 0;
    for (JsonObject n : notifications)
    {
        if (row >= MAX_ROWS) break;

        const char *source    = n["source"]    | "?";
        const char *sender    = n["sender"]    | "";
        const char *channel   = n["channel"]   | "";
        const char *preview   = n["preview"]   | "";
        const char *timestamp = n["timestamp"] | "";
        uint8_t priority      = n["priority"]  | 1;

        // Extract HH:MM from ISO8601 timestamp (chars 11-15)
        char timeStr[6] = "--:--";
        if (strlen(timestamp) >= 16)
        {
            timeStr[0] = timestamp[11];
            timeStr[1] = timestamp[12];
            timeStr[2] = ':';
            timeStr[3] = timestamp[14];
            timeStr[4] = timestamp[15];
            timeStr[5] = '\0';
        }

        drawRow(row, source, sender, channel, preview, timeStr, priority);
        row++;
    }

    lastDataMs = millis();
    _idle = false;
    _pollSent = false;
}

// Returns true once when the idle timeout first fires — caller sends a poll request.
// Resets automatically when drawScreen() is called with new data.
bool LCD::checkIdle()
{
    if (!_pollSent && millis() - lastDataMs > IDLE_TIMEOUT_MS)
    {
        _idle = true;
        _pollSent = true;
        return true;
    }
    return false;
}

// ── Private drawing helpers ───────────────────────────────────────────────────

void LCD::drawHeader()
{
    tft.setTextColor(DARKGREY, BLACK);
    tft.setTextSize(1);
    tft.setCursor(4, 6);
    tft.print("WEATHERBOX NOTIFICATIONS");
    // horizontal divider under header
    tft.drawFastHLine(0, HEADER_H, SCREEN_W, DARKGREY);
}

void LCD::drawRow(int rowIndex, const char *source, const char *sender,
                  const char *channel, const char *preview,
                  const char *timeStr, uint8_t priority)
{
    int y = HEADER_H + rowIndex * ROW_H;

    // Row border — yellow for priority 3, dark grey otherwise
    uint16_t borderColor = (priority >= 3) ? YELLOW : DARKGREY;
    tft.drawRect(0, y + 1, SCREEN_W, ROW_H - 2, borderColor);

    uint16_t badgeColor = sourceColor(source);

    // ── Source badge (top-left) ───────────────────────────────────────────────
    // Draw a filled rounded rect behind the source label
    char srcUpper[8];
    truncate(source, srcUpper, 7);
    for (uint8_t i = 0; srcUpper[i]; i++)
        if (srcUpper[i] >= 'a' && srcUpper[i] <= 'z') srcUpper[i] -= 32;
    // Shorten "MESSENGER" → "MSG"
    if (strncmp(srcUpper, "MESSENG", 7) == 0) strcpy(srcUpper, "MSG");

    int badgeW = strlen(srcUpper) * 6 + 6; // textSize=1: 6px/char
    tft.fillRoundRect(ROW_PAD, y + ROW_PAD, badgeW, 14, 3, badgeColor);
    tft.setTextColor(BLACK, badgeColor);
    tft.setTextSize(1);
    tft.setCursor(ROW_PAD + 3, y + ROW_PAD + 3);
    tft.print(srcUpper);

    // ── Channel name (next to badge) ──────────────────────────────────────────
    char chanBuf[33];
    truncate(channel, chanBuf, 32);
    tft.setTextColor(WHITE, BLACK);
    tft.setTextSize(1);
    tft.setCursor(ROW_PAD + badgeW + 6, y + ROW_PAD + 3);
    tft.print(chanBuf);

    // ── Timestamp (top-right) ─────────────────────────────────────────────────
    int timeX = SCREEN_W - strlen(timeStr) * 6 - ROW_PAD;
    tft.setTextColor(DARKGREY, BLACK);
    tft.setTextSize(1);
    tft.setCursor(timeX, y + ROW_PAD + 3);
    tft.print(timeStr);

    // ── Sender (middle line, larger text) ────────────────────────────────────
    char senderBuf[22];
    truncate(sender, senderBuf, 21); // textSize=2: 12px/char, max ~38 chars on 480px
    tft.setTextColor(badgeColor, BLACK);
    tft.setTextSize(2);
    tft.setCursor(ROW_PAD, y + ROW_PAD + 18);
    tft.print(senderBuf);

    // ── Preview text (bottom line) ────────────────────────────────────────────
    char previewBuf[73];
    truncate(preview, previewBuf, 72); // textSize=1: up to 80 chars on 480px
    tft.setTextColor(WHITE, BLACK);
    tft.setTextSize(1);
    tft.setCursor(ROW_PAD, y + ROW_PAD + 38);
    tft.print(previewBuf);

    // Row divider
    tft.drawFastHLine(0, y + ROW_H - 1, SCREEN_W, DARKGREY);
}


uint16_t LCD::sourceColor(const char *source)
{
    if (strncmp(source, "slack",     5) == 0) return MAGENTA;
    if (strncmp(source, "teams",     5) == 0) return BLUE;
    if (strncmp(source, "messenger", 9) == 0) return CYAN;
    if (strncmp(source, "iot",       3) == 0) return ORANGE;
    return WHITE;
}

void LCD::truncate(const char *src, char *dst, uint8_t maxLen)
{
    if (!src) { dst[0] = '\0'; return; }
    strncpy(dst, src, maxLen);
    dst[maxLen] = '\0';
}

LCD::~LCD() {}
