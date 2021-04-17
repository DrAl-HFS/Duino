# Duino/STM32

Requires STM32Duino addon package (x86 only) for Arduino GUI.

Quick & easy programming via STLink-V2 (clone) with debug logging over hardware serial (via FT232H or equivalent).

The MCU USB interface can, in principle, be used for CDC-ACM serial logging (e.g. "/dev/ttyACM0" on host), but seems 
unreliable. In particular after programming with STLink (CAVEAT: do not connect STLink clone power line when MCU is 
powered by USB) Arduino GUI seems unable to find "/dev/ttyACM0" until reset. Programming over USB-DFU interface also 
leads to frequent reset to restore functionality. This may well be a Linux specific Arduino-GUI problem, GTK and 
other toolkits have required frequent workarounds for this sort of problem. 

The efficient option for development is thus STLink + FT232H. 
