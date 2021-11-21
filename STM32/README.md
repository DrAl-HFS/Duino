# Duino/STM32

Requires STM32Duino addon package (x86 only) for Arduino GUI. A version for RPi/ARM host is in development
(stm32duino-raspberrypi) but does not seem to work with a clone STLink-V2. Also the time required to build a modest
test program is reminiscent of the VAX11/780 some of us learned C programming on several decades ago. This suggests
the toolchain needs a major tune-up. (A Pi4 is not radically slower than the decrepit but functional x86 build host:
a 2011 vintage dual core "i5" with 2GB DDR2-1066).

The x86 STM32Duino package allows quick & easy programming via STLink-V2 (clone) with debug logging over hardware
serial (via FT232H or equivalent).

The MCU USB interface can, in principle, be used for CDC-ACM serial logging (e.g. "/dev/ttyACM0" on host), but seems 
unreliable. In particular after programming with STLink (CAVEAT: do not connect STLink clone power line when MCU is 
powered by USB) Arduino GUI seems unable to find "/dev/ttyACM0" until reset. Programming over USB-DFU interface also 
leads to frequent reset to restore functionality. This may well be a Linux specific Arduino-GUI problem, GTK et.al.
have required frequent workarounds for this sort of thing. 

The efficient option for development is thus x86, STLink + FT232H. 
