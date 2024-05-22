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

Create a `src/config.h` file from `src/example_config` in the `eeprom_upload` directory and paste Certs and other values to
flash them into each ESP32 monitoring device.

You can find

AWS IOT Endpoint -> Settings (seems like you only have one endpoint per account?)
AWS Root CA Certificate - get this from https://docs.aws.amazon.com/iot/latest/developerguide/server-authentication.html#server-authentication-certs
AWS Device Private Key and Cert

- in Paramater Store
- or in the CDK deployment:
  - OutputCertPem -> Device Certificate
  - OutputPrivKey -> Device Private Key

Note! There are size limitations of the Preferences.h library. Only allows 15 characters for the keyname and 4000 characters for the value.

## How the code works

## Adding another monitoring Device

## Additional Feature/The Future

### Power
