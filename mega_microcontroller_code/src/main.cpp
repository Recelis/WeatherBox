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

  // Show idle screen after 5 minutes without new notifications
  myLCD.checkIdle();
}
