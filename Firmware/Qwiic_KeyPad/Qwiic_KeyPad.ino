/*
  An I2C based KeyPad
  By: Nathan Seidle
  SparkFun Electronics
  Date: January 21st, 2018
  License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

  Qwiic KeyPad is an I2C based key pad that records any button presses to a stack. If quered by the 
  master KeyPad will respond with the oldest button pressed along with the time since it was pressed.

  For example, if you Wire.request(75, 3) you'll get three bytes from KeyPad and they might read:
  byte 0: 0x34 - ASCII '4', button four was pressed
  byte 1: 0x01 - The MSB of a 16-bit unsigned integer
  byte 2: 0x5A - The LSB of a 16-bit unsigned integer

  0x015A = 346 so button 4 was pressed 346 milliseconds ago

  Feel like supporting our work? Buy a board from SparkFun!
  https://www.sparkfun.com/products/14641

  To install support for ATtiny84 in Arduino IDE: https://github.com/SpenceKonde/ATTinyCore/blob/master/Installation.md
  This core is installed from the Board Manager menu
  This core has built in support for I2C S/M and serial
  If you have Dave Mellis' ATtiny installed you may need to remove it from \Users\xx\AppData\Local\Arduino15\Packages

  To support 400kHz I2C communication reliably ATtiny84 needs to run at 8MHz. This requires user to
  click on 'Burn Bootloader' before code is loaded.

*/

#include <Wire.h>

#include <EEPROM.h>

#include <avr/sleep.h> //Needed for sleep_mode
#include <avr/power.h> //Needed for powering down perihperals such as the ADC/TWI and Timers

#define LOCATION_I2C_ADDRESS 0x01 //Location in EEPROM where the I2C address is stored
#define I2C_ADDRESS_DEFAULT 75
#define I2C_ADDRESS_JUMPER 74

#define COMMAND_CHANGE_ADDRESS 0xC7

//Variables used in the I2C interrupt so we use volatile
volatile byte setting_i2c_address = I2C_ADDRESS_DEFAULT; //The 7-bit I2C address of this KeyPad

byte responseBuffer[3]; //Used to pass data back to master
volatile byte responseSize = 1; //Defines how many bytes of relevant data is contained in the responseBuffer

//This struc keeps a record of all the button presses
#define BUTTON_STACK_SIZE 15
struct {
  byte button; //Which button was pressed?
  unsigned long buttonTime; //When?
} buttonEvents[BUTTON_STACK_SIZE];
volatile byte newestPress = 0;
volatile byte oldestPress = 0;

const byte addr = 9; //Addr jumper
const byte interruptPin = 7; //Pin goes low when a button event is available

#include <Keypad.h>   // by Mark Stanley and Alexander Brevig 
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'6', '4', '5'},
  {'9', '7', '8'},
  {'#', '*', '0'},
  {'3', '1', '2'}
};

//Pin X on keypad is connected to Y:
//KeyPad               7, 6, 4, 2
byte rowPins[ROWS] = {10, 5, 3, 1};
//Keypad              5, 3, 1
byte colPins[COLS] = {8, 2, 0};
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

void setup(void)
{
  pinMode(addr, INPUT_PULLUP);
  pinMode(interruptPin, OUTPUT); //Goes low when a button event is on the stack

  //Disable ADC
  ADCSRA = 0;
  
  //Disble Brown-Out Detect
  MCUCR = bit (BODS) | bit (BODSE);
  MCUCR = bit (BODS);

  //Power down various bits of hardware to lower power usage
  //set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();

  readSystemSettings(); //Load all system settings from EEPROM

  startI2C(); //Determine the I2C address we should be using and begin listening on I2C bus
}

void loop(void)
{
  //Check for new key presses
  char key = keypad.getKey();
  if (key) {
    buttonEvents[newestPress].button = key;
    buttonEvents[newestPress].buttonTime = millis();
    if (newestPress++ == BUTTON_STACK_SIZE) newestPress = 0; //Wrap variable
  }

  //Set interrupt pin as needed
  if(newestPress != oldestPress)
    digitalWrite(interruptPin, LOW); //We have events on the stack!
  else
    digitalWrite(interruptPin, HIGH); //No button events to report

  sleep_mode(); //Stop everything and go to sleep. Wake up if I2C event occurs.
}

//When KeyPad receives data bytes, this function is called as an interrupt
//The only valid command we can receive from the master is the change I2C adddress command
void receiveEvent(int numberOfBytesReceived)
{
  while (Wire.available())
  {
    //Record bytes to local array
    byte incoming = Wire.read();

    if (incoming == COMMAND_CHANGE_ADDRESS) //Set new I2C address
    {
      if (Wire.available())
      {
        setting_i2c_address = Wire.read();

        //Error check
        if (setting_i2c_address < 0x08 || setting_i2c_address > 0x77)
          continue; //Command failed. This address is out of bounds.

        EEPROM.write(LOCATION_I2C_ADDRESS, setting_i2c_address);

        //Our I2C address may have changed because of user's command
        startI2C(); //Determine the I2C address we should be using and begin listening on I2C bus
      }
    }
  }
}

//Send back a number of bytes via an array, max 32 bytes
//When KeyPad gets a request for data from the user, this function is called as an interrupt
//The interrupt will respond with different types of data depending on what response state we are in
//The user sets the response type based on bytes sent to KeyPad
void requestEvent()
{
  loadNextPressToArray();

  //Send response buffer
  for (byte x = 0 ; x < responseSize ; x++)
    Wire.write(responseBuffer[x]);
}

//Take the FIFO button press off the stack and load it into the transmit array
void loadNextPressToArray()
{
  if (oldestPress != newestPress)
  {
    responseBuffer[0] = buttonEvents[oldestPress].button;

    unsigned long timeSincePressed = millis() - buttonEvents[oldestPress].buttonTime;

    responseBuffer[1] = timeSincePressed >> 8; //MSB
    responseBuffer[2] = timeSincePressed; //LSB
    if (oldestPress++ == BUTTON_STACK_SIZE) oldestPress = 0;
  }
  else
  {
    //No new button presses. Respond with a blank record
    responseBuffer[0] = 0; //No button pressed
    responseBuffer[1] = 0;
    responseBuffer[2] = 0;
  }

  responseSize = 3;
}

//Reads the current system settings from EEPROM
//If anything looks weird, reset setting to default value
void readSystemSettings(void)
{
  //Read what I2C address we should use
  setting_i2c_address = EEPROM.read(LOCATION_I2C_ADDRESS);
  if (setting_i2c_address == 255)
  {
    setting_i2c_address = I2C_ADDRESS_DEFAULT; //By default, we listen for I2C_ADDRESS_DEFAULT
    EEPROM.write(LOCATION_I2C_ADDRESS, setting_i2c_address);
  }
}

//Begin listening on I2C bus as I2C slave using the global variable setting_i2c_address
void startI2C()
{
  Wire.end(); //Before we can change addresses we need to stop
  
  if (digitalRead(addr) == HIGH) //Default is HIGH, the jumper is open
    Wire.begin(setting_i2c_address); //Start I2C and answer calls using address from EEPROM
  else
    Wire.begin(I2C_ADDRESS_JUMPER); //Force address to I2C_ADDRESS_JUMPER if user has closed the solder jumper

  //The connections to the interrupts are severed when a Wire.begin occurs. So re-declare them.
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
}
