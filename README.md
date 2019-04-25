SparkFun Qwiic KeyPad - 12 Button
========================================

![SparkFun Qwiic Keypad](https://cdn.sparkfun.com//assets/parts/1/3/7/7/7/15290-SparkFun_Qwiic_Keypad_-_12_Button-01.jpg)

[*SparkFun Qwiic Keypad - 12 Button (COM-15290)*](https://www.sparkfun.com/products/15290)

Keypads are very handy input devices, but who wants to tie up seven GPIO pins, wire up a handful of pull-up resistors, and write firmware that wastes valuable processing time scanning the keys for inputs? The SparkFun Qwiic Keypad comes fully assembled and makes the development process for adding a 12 button keypad easy. No voltage translation or figuring out which I2C pin is SDA or SCL, just plug and go! Utilizing our handy Qwiic system, no soldering is required to connect it to the rest of your system. However, we still have broken out 0.1"-spaced pins in case you prefer to use a breadboard.

Each of the keypad's 12 buttons has been labeled 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, *, and # and has been formatted to into the same layout as a telephone keypad with each keypress resistance ranging between 10 and 150 Ohms. The Qwiic Keypad reads and stores the last 15 button presses in a First-In, First-Out (FIFO) stack, so you don’t need to constantly poll the keypad from your microcontroller. This information, then, is accessible through the Qwiic interface. The SparkFun Qwiic Keypad even has a software configurable I2C address so you can have multiple I2C devices on the same bus.

Repository Contents
-------------------

* **/Firmware** - The core sketch that runs Qwiic Keypad
* **/Hardware** - Eagle design files (.brd, .sch)

Documentation
--------------
* **[Hookup Guide](https://learn.sparkfun.com/tutorials/qwiic-keypad-hookup-guide)** - Hookup Guide for the SparkFun Qwiic Keypad - 12 Button
* **[Product Showcase - YouTube Video](https://youtu.be/QPRrk0tHk08)** - Product showcase video with Rob Reynolds introducing the product and showing a few examples.
* **[Library](https://github.com/sparkfun/SparkFun_Qwiic_Keypad_Arduino_Library)** - Arduino library for the SparkFun Qwiic Keypad - 12 Button
* **[Installing an Arduino Library](https://learn.sparkfun.com/tutorials/installing-an-arduino-library/)** - Basic instructions on installing an Arduino library

Product Versions
--------------
* [COM-15290](https://www.sparkfun.com/products/15290) - SparkFun version
* [SPX-14836](https://www.sparkfun.com/products/retired/14836) - SparkX version

License Information
-------------------

This product is _**open source**_! 

Please review the LICENSE.md file for license information.

If you have any questions or concerns on licensing, please contact technical support on our [SparkFun forums](https://forum.sparkfun.com/viewforum.php?f=152).

Please use, reuse, and modify these files as you see fit. Please maintain attribution to SparkFun Electronics and release under the same license.

Distributed as-is; no warranty is given.

- Your friends at SparkFun.
