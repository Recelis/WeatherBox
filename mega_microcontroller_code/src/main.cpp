#include <Arduino.h>
#include <Communication.h>
#include <LCD.h>

Communication myCommunication;
LCD myLCD;

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  myLCD.startScreen();
  myLCD.refreshScreen();
}

void loop()
{
  // put your main code here, to run repeatedly:
  bool newData = myCommunication.receiveData();
  // print screen only if new data is received
  if (newData)
  {
    Serial.println("Received new data");
    myLCD.refreshScreen(); // refresh screen
    myLCD.drawScreen(myCommunication.getReceivedChars());
    myCommunication.setNewData(false);
  }
}