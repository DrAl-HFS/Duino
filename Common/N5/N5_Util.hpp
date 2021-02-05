// Duino/Common/N5/N5_Util.hpp - Nordic nRF5* definitions wrapper and misc utils
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Jan 2021

#ifndef N5_UTIL
#define N5_UTIL

// Arduino build env target variant symbols
// TODO : find more broadly applicable symbols (e.g. processor ID)
#ifdef _VARIANT_BBC_MICROBIT_
#include <nrf51.h>
#include <nrf51_bitfields.h>
#endif // _VARIANT_BBC_MICROBIT_

#ifdef _MICROBIT_V2_
#include <nrf52.h>
#include <nrf52_bitfields.h>
#endif // _MICROBIT_V2_


/***/


#endif // N5_UTIL
