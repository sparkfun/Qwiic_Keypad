/*
  An I2C based KeyPad
  By: Nathan Seidle
  SparkFun Electronics
  Date: January 21st, 2018
  License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

  Feel like supporting our work? Buy a board from SparkFun!
  https://www.sparkfun.com/products/14641

  This example prints which button was pressed and when it was presed.
  
  Qwiic KeyPad records any button presses to a stack. If quered by the master KeyPad will
  respond with the oldest button pressed along with the time since it was pressed.
*/

#include <Wire.h>

#define QWIIC_KEYPAD_ADDRESS 75 //75 is default, 74 if jumper is closed

char button = 0; //Button '0' to '9' and '*' and '#'
unsigned long timeSincePress = 0; //Number of milliseconds since this button was pressed

void setup(void)
{
  Wire.begin();
  Wire.setClock(400000); //High speed I2C is supported!
  
  Serial.begin(9600);
  Serial.println("Read Qwiic KeyPad Example");
}

void loop(void)
{
  readKeyPad(); //Sets the button and timeSincePress global variables
  if(button == -1)
  {
    Serial.println("No keypad detected");
    delay(1000);
  }
  else if(button == 0)
  {
    Serial.println("No button has been pressed.");
    delay(1000);
  }
  else
  {
    Serial.println("Button '" + String(button) + "' was pressed " + String(timeSincePress) + " milliseconds ago.");
  }
  
  //Do something else. Don't call readKeyPad a ton otherwise you'll tie up the I2C bus
  delay(25); //25 is good, more is better
  
}

//Get the latest button and time stamp from keypad
void readKeyPad()
{
  Wire.requestFrom((uint8_t)QWIIC_KEYPAD_ADDRESS, (uint8_t)3); //Keypad responds with 3 bytes every time
  
  button = Wire.read();
  timeSincePress = Wire.read() << 8; //Time is a 16 bit unsigned int. 65,535 is max (or 65.535 seconds)
  timeSincePress |= Wire.read();
}

