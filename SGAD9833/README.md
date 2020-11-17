# Duino/AD9833

Yet another AD9833 signal generator with host control over serial link.
Noobs should probably start out with the version by Peter Balch -

 https://www.instructables.com/Signal-Generator-AD9833/

which is easy to follow and has been (badly) copied many times, often
without observing licences or giving credit.

The offering here is developed from first principles with the objectives
of better flexibility and higher performance, at the expense of greater
complexity. C++ is quite useful in making this more manageable, although
it falls down with regard to interrupt handling. Key features are:-

*   Cyclic executive main loop, processing events.

*   Interrupt triggered events - timer at 1ms interval.

*   Hardware SPI communication with AD9833 using 8MHz bus clock - expected
latency is <3us per 16bit message (untested).

*   UART communication with host, providing a simple but adequately flexible
command interface e.g. 

	- "S" or "T" - sine or triangle waveform output from DAC (0.3Vpp) 
	- "C" or "D" - clock or double-rate-clock direct output (Vcc 3~5V)
	- "4.5E3" or "4.5k" or "4k5" - 4.5kHz signal
	- "1k,10k,100m" - sweep from 1kHz to 10kHz every 100msec

