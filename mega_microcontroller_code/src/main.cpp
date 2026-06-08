#include <Arduino.h>
#include <Communication.h>
#include <LCD.h>

Communication myCommunication;
LCD myLCD;

void setup()
{
  Serial.begin(115200);
  myLCD.startScreen();
  myLCD.refreshScreen();
}

void loop()
{
  bool newData = myCommunication.receiveData();
  if (newData)
  {
    Serial.println("Received notification batch");
    myLCD.drawScreen(myCommunication.getReceivedChars());
    myCommunication.setNewData(false);
  }

  // After 5 minutes with no new data, ask the ESP32 to re-send its last batch
  if (myLCD.checkIdle())
  {
    Serial.println("Idle timeout — polling ESP32 for last batch");
    myCommunication.sendPollRequest();
  }
}
