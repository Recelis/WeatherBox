#ifndef WIFI_PROVISIONER_H
#define WIFI_PROVISIONER_H

#include <WebServer.h>
#include <DNSServer.h>

// Starts a SoftAP captive portal so the user can enter new WiFi credentials
// from any phone or laptop. Blocks inside begin() until credentials are saved,
// then reboots the ESP32 automatically.
class WiFiProvisioner
{
public:
    void begin();

private:
    WebServer _server{80};
    DNSServer _dns;

    void _startAP();
    void _registerRoutes();
    String _buildPage(const String &message = "", const String &msgClass = "");
    String _scanNetworks();
    void _handleRoot();
    void _handleSave();
    void _handleCaptiveRedirect();
};

#endif
