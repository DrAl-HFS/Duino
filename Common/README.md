# Duino/Common
A collection of c++ header files providing shareable code for Arduino IDE builds.
Presently these all relate to the AD9833 signal generator project but will eventually
be shared as various hacky projects are reorganised and refactored. Some device
definition & utility code exists in the "mainline" <Common/MBD> repository: a Linux
soft link to the relevant folder on disk is used to make the MBD folder appear within
<Duino/Common/> e.g.

	ln -s $DVL_ROOT/Common/MBD $DVL_ROOT/Duino/Common/

