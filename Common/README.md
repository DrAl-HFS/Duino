# Duino/Common

Utility code shared between projects. Some is portable while the majority is architecture specific 
due to the performance advantage offered by manipulating peripheral devices directly.

Some device definition & utility code exists in the "mainline" <Common/MBD> repository: a Linux
soft link to the relevant folder on disk is used to make the MBD folder appear within
<Duino/Common/> e.g.

        ln -s $DVL_ROOT/Common/MBD $DVL_ROOT/Duino/Common/


## Ungrouped modules
M0_Util	- Hacks for ARM Cortex MCU with limited ALU (e.g. M0+)
RotEnc	- Rotary Encoder "driver" code suitable for user control input (polled ~1kHz).
Wave8	- 8bit waveform synthesis hacks.
