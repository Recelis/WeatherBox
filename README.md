# WeatherBox (fork from ESP32Monitoring)

This is a ESP32 project for getting an up to date weather forecast connected to a Arduino MEGA for displaying them onto a screen. It was forked from ESP32Monitoring to later be able to display monitoring readings from other IOT devices.

![weatherbox](https://github.com/user-attachments/assets/9d4d59f5-395f-421d-8e99-c8f0283729b1)

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

You can set the exact versions as well as allowing for minor upgrades. These packages can be found in the PlatformIO Registry.

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

Note! We use `cdk-iot-core-certificates` to set up multiple Things and their certs in AWS. While this is powerful, it also gives each Thing really wide range authorization and there is no way to set a more fine-grained policies.

## How the code works

### `cdk-iot-core-certificates`

This is a L3 construct that creates a thing and their cert in AWS with CDK. It was created by AWS so it is actually an official package.

### `@aws-solutions-constructs/aws-iot-lambda`

This is a AWS Solutions Construct which integrates a Lambda to a MQTT topic. While it does help make it easier to use an existing lambda, it may be better to just use a IoT Topic Rule with an action to add the message to SQS.

## Adding another monitoring Device
Things are defined in the `create-things.ts` file in the cdk folder. You can add a new thing by adding to the things array. Please note, there is no uniqueness check on the thing suffix. You have to do this manually.

```Typescript
interface ICreateThing {
  suffix: string;
}

export const things: ICreateThing[] = [
  {
    suffix: "Monitor1",
  },
  {
    suffix: "Monitor2",
  },
];
```
