# Duino/AVR/TinyRTC

Testing of TinyRTC (Maxim/Dallas DS1307 RTC + Microchip/Atmel AT24C EEPROM) module.

* For versions with LR2032 charge circuit design defect:
 - remove D1, R4, R5 & replace R6 with conductor to restore functionality.

* To prevent RTC battery "back powering" the MCU:
 - set SDA & SCL pins to high impedance when not in use
 - power module from an IO pin, set to high impedance when power fails (detection?)
