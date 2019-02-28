/*
  An I2C based KeyPad
  By: Nathan Seidle
  SparkFun Electronics
  Date: January 21st, 2018
  License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

  Feel like supporting our work? Buy a board from SparkFun!
  https://www.sparkfun.com/products/14641

  This example demonstrates how to change the I2C address via software.

  Note: To change the address you can also open the on-board jumper. This will force the address to 74 (0x4A).

  Note: If you change the address to something unknown you can either open the jumper (goes to address 0x4A) or
  use the I2C scanner sketch (Example 4).
*/

#include <Wire.h>

byte keypadAddress = 75; //75 is default, 74 if jumper is closed
byte newAddress = 55; //Must be 0x08 <= newAddress <= 0x77

void setup(void)
{
  Wire.begin();
  
  Serial.begin(9600);
  Serial.println("Qwiic KeyPad Change Address Example");
  Serial.println("Press a key to begin");

  while(Serial.available()) Serial.read(); //Clear buffer
  while(!Serial.available()) delay(1); //Wait for user to press button  

  //The new address must be 0x08 <= address <= 0x77
  if(changeKeyPadAddress(keypadAddress, newAddress) == true) //Old address, new address
  {
    keypadAddress = newAddress;
    Serial.println("Address successfully changed to 0x" + String(keypadAddress, HEX));
  }
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

//Change the I2C address from one address to another
boolean changeKeyPadAddress(byte oldAddress, byte newAddress)
{
  Wire.beginTransmission(oldAddress); //Communicate using the old address
  Wire.write(0xC7); //0xC7 is the register location on the KeyPad to change its I2C address
  Wire.write(newAddress); //Go to the new address
  if (Wire.endTransmission() != 0)
  {
    //Sensor did not ACK
    Serial.println("Error: Sensor did not ack");
    return(false);
  }
  return(true);
}

