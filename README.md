SparkFun Qwiic KeyPad
========================================

![SparkFun Qwiic Keypad](https://cdn.sparkfun.com//assets/parts/1/2/8/5/4/Qwiic-Keypad-Example.jpg)

[*SparkX Qwiic Keypad (SPX-14836)*](https://www.sparkfun.com/products/14836)

Keypads are very handy input devices. And there are many great libraries written to interface to keypads! But who wants to tie up 7 GPIOs, have a handful of pull up resistors, and write firmware that scans the keys taking up valuable megahertz? Let’s make it easier! The Qwiic Keypad uses very simple I2C commands to read what button was pressed. It also implements a stack with time stamps for each key press so you don’t need to constantly poll the keypad. Qwiic Keypad even has a configurable I2C address so you can have multiple keypads on the same bus!

Qwiic Keypad is very low power and uses less than 4mA at 3.3V. There are jumpers on the board allowing the user to select between different I2C addresses as well as to remove the I2C pull up resistors if needed.

The Qwiic Keypad comes fully assembled and uses the simple [Qwiic interface](https://www.sparkfun.com/qwiic). No soldering, no voltage translation, no figuring out which pin is SDA or SCL, just plug and go!

Repository Contents
-------------------

* **/Examples** - A number of examples to show how to read buttons with time, change the I2C address and scan for I2C devices
* **/Firmware** - The core sketch that runs Qwiic Keypad
* **/Hardware** - Eagle design files (.brd, .sch)

License Information
-------------------

This product is _**open source**_! 

Please review the LICENSE.md file for license information. 

If you have any questions or concerns on licensing, please contact techsupport@sparkfun.com.

Please use, reuse, and modify these files as you see fit. Please maintain attribution to SparkFun Electronics and release any derivative under the same license.

Distributed as-is; no warranty is given.

- Your friends at SparkFun.
