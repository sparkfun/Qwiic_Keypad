/*
  An I2C based KeyPad
  By: Nathan Seidle
  SparkFun Electronics
  Date: January 21st, 2018
  License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

  Updated by: Pete Lewis
  SparkFun Electronics
  Date: March 16th, 2019

  Qwiic KeyPad is an I2C based key pad that records any button presses to a stack. To read buttons off the
  stack, the master must first command the keypad to load the fifo register, then read the fifo register.

  There is also an accompanying Arduino Library located here:
  https://github.com/sparkfun/SparkFun_Qwiic_Keypad_Arduino_Library

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
                      // Click here to get the library: http://librarymanager/All#Keypad_matrix_style
                      // Note, if the search above doesn't work, try just "keypad" and find the correct one.
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'6', '4', '5'},
  {'9', '7', '8'},
  {'#', '*', '0'},
  {'3', '1', '2'}
};


#if defined(__AVR_ATmega328P__)
//Hardware connections while developing with an Uno
//Note, could not use pins 0 and 1 on uno, due to conflicting RX/TX debug
//Pin X on keypad is connected to Y:
//KeyPad               7, 6, 4, 2
byte rowPins[ROWS] = {10, 5, 3, 6};
//Keypad              5, 3, 1
byte colPins[COLS] = {8, 2, 4};

#elif defined(__AVR_ATtiny84__)
//Hardware connections for the final design
//Pin X on keypad is connected to Y:
//KeyPad               7, 6, 4, 2
byte rowPins[ROWS] = {10, 5, 3, 1};
//Keypad              5, 3, 1
byte colPins[COLS] = {8, 2, 0};

#endif
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

//Global variables
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//Variables used in the I2C interrupt so we use volatile

//This is the pseudo register map of the product. If user asks for 0x02 then get the 3rd
//byte inside the register map.
//5602/118 on ATtiny84 prior to conversion
//Approximately 4276/156 on ATtiny84 after conversion
struct memoryMap {
  byte id;                    // Reg: 0x00 - Default I2C Address
  byte firmwareMajor;         // Reg: 0x01 - Firmware Number
  byte firmwareMinor;         // Reg: 0x02 - Firmware Number
  byte fifo_button;         // Reg: 0x03 - oldest button (aka the "first" button pressed)
  byte fifo_timeSincePressed_MSB;  // Reg: 0x04 - time in milliseconds since the buttonEvent occured (MSB)
  byte fifo_timeSincePressed_LSB;  // Reg: 0x05 - time in milliseconds since the buttonEvent occured (LSB)
  byte updateFIFO;            // Reg: 0x06 - "command" from master, set bit0 to command fifo increment
  byte i2cAddress;            // Reg: 0x07 - Set I2C New Address (re-writable).
};

//These are the default values for all settings
volatile memoryMap registerMap = {
  .id = I2C_ADDRESS_DEFAULT, //Default I2C Address (0x20)
  .firmwareMajor = 0x01, //Firmware version. Helpful for tech support.
  .firmwareMinor = 0x00,
  .fifo_button = 0,
  .fifo_timeSincePressed_MSB = 0,
  .fifo_timeSincePressed_LSB = 0,
  .updateFIFO = 0,
  .i2cAddress = I2C_ADDRESS_DEFAULT,
};

//This defines which of the registers are read-only (0) vs read-write (1)
memoryMap protectionMap = {
  .id = 0x00,
  .firmwareMajor = 0x00,
  .firmwareMinor = 0x00,
  .fifo_button = 0x00,
  .fifo_timeSincePressed_MSB = 0x00,
  .fifo_timeSincePressed_LSB = 0x00,
  .updateFIFO = 0x01, // allow read-write on bit0 to "command" fifo increment update
  .i2cAddress = 0xFF,
};

//Cast 32bit address of the object registerMap with uint8_t so we can increment the pointer
uint8_t *registerPointer = (uint8_t *)&registerMap;
uint8_t *protectionPointer = (uint8_t *)&protectionMap;

volatile byte registerNumber; //Gets set when user writes an address. We then serve the spot the user requested.

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

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

#if defined(__AVR_ATmega328P__)
  Serial.begin(115200);
  Serial.println("Qwiic Keypad");
  Serial.print("Address: 0x");
  Serial.print(registerMap.i2cAddress, HEX);
  Serial.println();
  Serial.print("EEPROM READ I2C Address:");
  Serial.print(EEPROM.read(LOCATION_I2C_ADDRESS), HEX);
  Serial.println();
  print_registerMap();
#endif

}

void loop(void)
{
  //Check for new key presses
  char key = keypad.getKey();
  if (key) {
    //Check for buffer overrun
    // if oldest press is trailing just one behind, then we also will need to increment
    if (oldestPress == (newestPress + 1)) oldestPress++;
    if ( (newestPress == (BUTTON_STACK_SIZE - 1)) && (oldestPress == 0) ) oldestPress++;
    if (oldestPress == BUTTON_STACK_SIZE) oldestPress = 0; //still need to wrap if it happens

    newestPress++;
    if (newestPress == BUTTON_STACK_SIZE) newestPress = 0; //Wrap variable

    buttonEvents[newestPress].button = key;
    buttonEvents[newestPress].buttonTime = millis();

    digitalWrite(interruptPin, HIGH); //Set Int HIGH, to cause a FALLING edge later
    //Note, this will be set LOW again at bottom of main loop.
    //This FALLING edge is useful for hardware INT on your project's master IC.

#if defined(__AVR_ATmega328P__)
    Serial.print(char(buttonEvents[newestPress].button));
    print_registerMap();
    print_buttonEvents();
#endif
  }

  //Set interrupt pin as needed
  if (newestPress != oldestPress)
    digitalWrite(interruptPin, LOW); //We have events on the stack!
  else
    digitalWrite(interruptPin, HIGH); //No button events to report

  sleep_mode(); //Stop everything and go to sleep. Wake up if I2C event occurs.
}

//When Qwiic Keypad receives data bytes from Master, this function is called as an interrupt
//(Serves rewritable I2C address and updateFifo command)
void receiveEvent(int numberOfBytesReceived)
{
  if(numberOfBytesReceived > 0) // ensure we received valid byte or bytes - note, an I2C scan does not send anything, so without this, it would overright registerNumber with Zero.
  {
  registerNumber = Wire.read(); //Get the memory map offset from the user

  //Begin recording the following incoming bytes to the temp memory map
  //starting at the registerNumber (the first byte received)
  for (byte x = 0 ; x < numberOfBytesReceived - 1 ; x++)
  {
    byte temp = Wire.read(); //We might record it, we might throw it away

    if ( (x + registerNumber) < sizeof(memoryMap))
    {
      //Clense the incoming byte against the read only protected bits
      //Store the result into the register map
      *(registerPointer + registerNumber + x) &= ~*(protectionPointer + registerNumber + x); //Clear this register if needed
      *(registerPointer + registerNumber + x) |= temp & *(protectionPointer + registerNumber + x); //Or in the user's request (clensed against protection bits)
    }
  }

  recordSystemSettings();
  }
}

//Send back a number of bytes via an array, max 32 bytes
//When KeyPad gets a request for data from the user, this function is called as an interrupt
//The interrupt will respond with different types of data depending on what response state we are in
//The user sets the response type based on bytes sent to KeyPad
void requestEvent()
{
  if (registerMap.updateFIFO & (1 << 0))
  {
    // clear command bit
    registerMap.updateFIFO &= ~(1 << 0);

    // update fifo, that is... copy oldest button (and buttonTime) into fifo register (ready for reading)
    loadFifoRegister();
  }

#if defined(__AVR_ATmega328P__)
  print_registerMap();
  print_buttonEvents();
#endif

  Wire.write((registerPointer + registerNumber), sizeof(memoryMap) - registerNumber);
}

//Take the FIFO button press off the stack and load it into the fifo register (ready for reading)
void loadFifoRegister()
{
  if (oldestPress != newestPress)
  {
    oldestPress++;
    if (oldestPress == BUTTON_STACK_SIZE) oldestPress = 0;

    registerMap.fifo_button = buttonEvents[oldestPress].button;

    unsigned long timeSincePressed = millis() - buttonEvents[oldestPress].buttonTime;

    registerMap.fifo_timeSincePressed_MSB = (timeSincePressed >> 8);
    registerMap.fifo_timeSincePressed_LSB = timeSincePressed;
  }
  else
  {
    //No new button presses. load blank records
    registerMap.fifo_button = 0;
    registerMap.fifo_timeSincePressed_MSB = 0;
    registerMap.fifo_timeSincePressed_LSB = 0;
  }
}

//Reads the current system settings from EEPROM
//If anything looks weird, reset setting to default value
void readSystemSettings(void)
{
  //Read what I2C address we should use
  registerMap.i2cAddress = EEPROM.read(LOCATION_I2C_ADDRESS);
  if (registerMap.i2cAddress == 255)
  {
    registerMap.i2cAddress = I2C_ADDRESS_DEFAULT; //By default, we listen for I2C_ADDRESS_DEFAULT
    EEPROM.write(LOCATION_I2C_ADDRESS, registerMap.i2cAddress);
  }
}

//Begin listening on I2C bus as I2C slave using the global variable registerMap.i2cAddress
void startI2C()
{
  Wire.end(); //Before we can change addresses we need to stop

  if (digitalRead(addr) == HIGH) //Default is HIGH, the jumper is open
    Wire.begin(registerMap.i2cAddress); //Start I2C and answer calls using address from EEPROM
  else
    Wire.begin(I2C_ADDRESS_JUMPER); //Force address to I2C_ADDRESS_JUMPER if user has closed the solder jumper

  //The connections to the interrupts are severed when a Wire.begin occurs. So re-declare them.
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
}

#if defined(__AVR_ATmega328P__)
void print_registerMap()
{
  Serial.println("registerMap contents:");
  for (byte i = 0 ; i < sizeof(memoryMap) ; i++)
  {
    Serial.println(*(registerPointer + i));
  }
}

void print_buttonEvents()
{
  Serial.println("buttonEvents[] contents: button, buttonTime");
  for (byte i = 0 ; i < BUTTON_STACK_SIZE ; i++)
  {
    Serial.print("[");
    Serial.print(i);
    Serial.print("]:");
    Serial.print(char(buttonEvents[i].button));
    Serial.print(", ");
    Serial.println(buttonEvents[i].buttonTime);
  }
}
#endif

//If the current setting is different from that in EEPROM, update EEPROM
void recordSystemSettings(void)
{
  //I2C address is byte
  byte i2cAddr;

  //Error check the current I2C address
  if (registerMap.i2cAddress < 0x08 || registerMap.i2cAddress > 0x77)
  {
    //User has set the address out of range
    //Go back to defaults
    registerMap.i2cAddress = I2C_ADDRESS_DEFAULT;
  }

  //Read the value currently in EEPROM. If it's different from the memory map then record the memory map value to EEPROM.
  EEPROM.get(LOCATION_I2C_ADDRESS, i2cAddr);
  if (i2cAddr != registerMap.i2cAddress)
  {
    EEPROM.write(LOCATION_I2C_ADDRESS, registerMap.i2cAddress);
    startI2C(); //Determine the I2C address we should be using and begin listening on I2C bus
    //Serial.print("New Address: 0x");
    //Serial.println(registerMap.i2cAddress, HEX);
  }
}
