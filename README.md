# ESP32Monitoring

## Setup

### Setup PlatformIO to program on a real editor

A new platformio project was created in the repo. I chose Espressif ESP32 Dev module as my board and Arduino as my development framework. The settings for this can be easily changed in the `platformio.ini` page.

#### Library Setup

The `lib_deps` field in `platform.ini` manages your imported libraries.

```ini
lib_deps=
    256dpi/MQTT @ ^2.5.2
    bblanchon/ArduinoJson @^7.0.4
```

You can set the exact versions as well as allowing for minor upgrades.

### Set up AWS CDK to deploy your monitoring!

## How the code works

## Adding another monitoring Device

## Additional Feature/The Future

### Power
