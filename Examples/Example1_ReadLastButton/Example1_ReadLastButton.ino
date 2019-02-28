/*
  An I2C based KeyPad
  By: Nathan Seidle
  SparkFun Electronics
  Date: January 21st, 2018
  License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

  Feel like supporting our work? Buy a board from SparkFun!
  https://www.sparkfun.com/products/14641

  This example prints which button was pressed. Press * for space and # for new line.
  
  Qwiic KeyPad records any button presses to a stack. If quered by the master KeyPad will
  respond with the oldest button pressed along with the time since it was pressed.
*/

#include <Wire.h>

byte keypadAddress = 75; //75 (0x4B) is default, 74 if jumper is closed

void setup(void)
{
  Wire.begin();
  
  Serial.begin(9600);
  Serial.println("Read Qwiic KeyPad Example");
  Serial.println("Press a button: * to do a space. # to go to next line.");
}

void loop(void)
{
  char button = readKeyPad();

  if(button == -1)
  {
    Serial.println("No keypad detected");
    delay(1000);
  }
  else if(button != 0)
  {
    if(button == '#') Serial.println();
    else if(button == '*') Serial.print(" ");
    else Serial.print(button);
  }

  //Do something else. Don't call readKeyPad a ton otherwise you'll tie up the I2C bus
  delay(25); //25 is good, more is better
}

//Get the latest button
char readKeyPad()
{
  Wire.requestFrom((uint8_t)keypadAddress, (uint8_t)1);
  return(Wire.read());
}

