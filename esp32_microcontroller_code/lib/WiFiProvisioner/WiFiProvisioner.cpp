#include "WiFiProvisioner.h"
#include <WiFi.h>
#include <Preferences.h>

#define AP_SSID    "WeatherBox-Setup"
#define DNS_PORT   53

// Minimal mobile-friendly page. %OPTIONS% and %MSG% are replaced at runtime.
static const char PAGE_TEMPLATE[] =
    "<!DOCTYPE html><html><head>"
    "<meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>WeatherBox Setup</title>"
    "<style>"
    "body{font-family:sans-serif;max-width:420px;margin:40px auto;padding:0 20px;background:#111;color:#eee}"
    "h1{font-size:22px;margin-bottom:4px}p.sub{color:#888;margin-top:0;font-size:14px}"
    "label{display:block;margin-bottom:4px;font-size:14px;color:#aaa}"
    "select,input{width:100%;padding:10px;margin:0 0 16px;border-radius:6px;border:1px solid #444;"
    "background:#222;color:#eee;font-size:16px;box-sizing:border-box}"
    "button{width:100%;padding:12px;background:#1a6aff;color:#fff;border:none;border-radius:6px;"
    "font-size:16px;cursor:pointer}"
    ".msg{padding:10px 14px;border-radius:6px;margin-bottom:16px;font-size:14px}"
    ".err{background:#3a0000;color:#ff8080}"
    ".ok{background:#003a10;color:#80ff80}"
    "</style></head><body>"
    "<h1>WeatherBox Setup</h1>"
    "<p class='sub'>Connect to your WiFi network</p>"
    "%MSG%"
    "<form method='POST' action='/save'>"
    "<label>Network</label>"
    "<select name='ssid'>%OPTIONS%</select>"
    "<label>Password</label>"
    "<input type='password' name='password' placeholder='WiFi password'>"
    "<button type='submit'>Connect</button>"
    "</form></body></html>";

// ── Public ────────────────────────────────────────────────────────────────────

void WiFiProvisioner::begin()
{
    Serial.println("Entering provisioning mode");
    _startAP();
    _dns.start(DNS_PORT, "*", WiFi.softAPIP());
    _registerRoutes();
    _server.begin();
    Serial.print("Provisioning portal at http://");
    Serial.println(WiFi.softAPIP());

    // Block here — the portal handles everything via callbacks until reboot
    while (true)
    {
        _dns.processNextRequest();
        _server.handleClient();
        delay(2);
    }
}

// ── Private ───────────────────────────────────────────────────────────────────

void WiFiProvisioner::_startAP()
{
    WiFi.disconnect(true);
    delay(100);
    WiFi.mode(WIFI_AP_STA); // STA needed for the network scan
    WiFi.softAP(AP_SSID);
    delay(500); // give the AP time to come up
    Serial.print("SoftAP started: ");
    Serial.println(AP_SSID);
}

void WiFiProvisioner::_registerRoutes()
{
    _server.on("/",        HTTP_GET,  [this]() { _handleRoot(); });
    _server.on("/save",    HTTP_POST, [this]() { _handleSave(); });

    // Captive portal detection endpoints — iOS, Android, Windows all probe
    // different URLs; redirect them all to the provisioning page
    _server.onNotFound([this]() { _handleCaptiveRedirect(); });
}

void WiFiProvisioner::_handleRoot()
{
    _server.send(200, "text/html", _buildPage());
}

void WiFiProvisioner::_handleSave()
{
    String ssid     = _server.arg("ssid");
    String password = _server.arg("password");

    if (ssid.isEmpty())
    {
        _server.send(200, "text/html", _buildPage("Please select a network.", "err"));
        return;
    }

    Serial.print("Trying to connect to: ");
    Serial.println(ssid);

    // Stay in AP_STA mode so the phone keeps its connection to the AP
    // and can receive the success/failure response page.
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000)
    {
        _dns.processNextRequest();
        _server.handleClient();
        delay(100);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Connection failed, staying in provisioning mode");
        WiFi.disconnect();
        _server.send(200, "text/html",
            _buildPage("Wrong password or network not found. Try again.", "err"));
        return;
    }

    // Success — save credentials before sending the response
    Serial.println("Connected! Saving credentials");
    Preferences prefs;
    prefs.begin("ESP32Monitoring", false);
    prefs.putString("WIFI_SSID",     ssid);
    prefs.putString("WIFI_PASSWORD", password);
    prefs.end();

    // Send response and keep handling requests for 3 seconds so the
    // phone receives the page before the AP disappears on reboot
    _server.send(200, "text/html",
        _buildPage("Connected! Rebooting WeatherBox&hellip;", "ok"));

    unsigned long flush = millis();
    while (millis() - flush < 3000)
    {
        _server.handleClient();
        delay(10);
    }

    ESP.restart();
}

void WiFiProvisioner::_handleCaptiveRedirect()
{
    // Redirect every unknown path to the provisioning page —
    // this is what causes phones to pop up the captive portal dialog
    _server.sendHeader("Location", "http://192.168.4.1/", true);
    _server.send(302, "text/plain", "");
}

String WiFiProvisioner::_scanNetworks()
{
    Serial.println("Scanning networks...");
    int n = WiFi.scanNetworks();
    String options = "";

    if (n <= 0)
    {
        options = "<option value=''>No networks found — refresh the page</option>";
        return options;
    }

    // Build a sorted index by RSSI (strongest first) using insertion sort
    int idx[n];
    for (int i = 0; i < n; i++) idx[i] = i;
    for (int i = 1; i < n; i++)
    {
        int key = idx[i];
        int j   = i - 1;
        while (j >= 0 && WiFi.RSSI(idx[j]) < WiFi.RSSI(key))
        {
            idx[j + 1] = idx[j];
            j--;
        }
        idx[j + 1] = key;
    }

    // Build option list, skipping hidden SSIDs and duplicates
    String seen = "|";
    for (int i = 0; i < n; i++)
    {
        String ssid = WiFi.SSID(idx[i]);
        if (ssid.isEmpty() || seen.indexOf("|" + ssid + "|") >= 0) continue;
        seen += ssid + "|";

        int rssi    = WiFi.RSSI(idx[i]);
        bool locked = (WiFi.encryptionType(idx[i]) != WIFI_AUTH_OPEN);
        String label = ssid + " (" + String(rssi) + " dBm" + (locked ? " *" : "") + ")";
        options += "<option value='" + ssid + "'>" + label + "</option>";
    }

    WiFi.scanDelete();
    return options;
}

String WiFiProvisioner::_buildPage(const String &message, const String &msgClass)
{
    String msg = "";
    if (!message.isEmpty())
        msg = "<div class='msg " + msgClass + "'>" + message + "</div>";

    String options = _scanNetworks();

    String page = PAGE_TEMPLATE;
    page.replace("%MSG%",     msg);
    page.replace("%OPTIONS%", options);
    return page;
}
