:Begin
avrdude -Cavrdude.conf -v -pattiny84 -cusbtiny -Uflash:w:Qwiic_KeyPad.ino.hex:i -e -Uefuse:w:0xFF:m -Uhfuse:w:0b11010101:m -Ulfuse:w:0xE2:m 
pause
goto Begin