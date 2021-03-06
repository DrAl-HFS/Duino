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

*   Cyclic executive main loop, state machine processing.

*   Timer interrupt control - events processed at 1ms intervals.

*   Hardware SPI communication with AD9833 using 8MHz bus clock - minimum
latency is around 40 CPU cycles (2.5us @ 16MHz) per 16bit message.

*   Frequency sweep up/down in uniform steps per millisecond. Work on fast
nonlinear stepping (requires fixed point calculation) is ongoing.

*   Chirp (sub-millisecond sweep) generation under development.

*   Pulse counting on T1 pin can used to estimate frequency of AD9833
clock output. (DAC output requires conditioning from waveform to logic pulse.)

*   ADC input on A0 can monitor AD9833 output, A8 for AVR temperature.

*   Settable "real-time" clock with hh:mm:ss.mmm resolution.

*   UART communication with host, providing a simple but adequately flexible
command interface e.g. 

	- "S" or "T" - sine or triangle waveform output from DAC (0.6Vpp) 
	- "C" or "L" - clock or half-rate-clock direct output (Vcc 3~5V)
	- "4.5E3" or "4.5k" or "4k5" - 4.5kHz signal
	- "1k,10k,100m" - sweep from 1kHz to 10kHz every 100msec
	- "K13:25:30" or "K132530" - set clock
