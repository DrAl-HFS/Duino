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
#ifndef TARGET_UBITV1
#define TARGET_UBITV1
#endif //
#ifndef TARGET_NRF51
#define TARGET_NRF51
#endif //
#endif // _VARIANT_BBC_MICROBIT_

#ifdef _MICROBIT_V2_
/* seems to be set up automagically - precompiled ?
#include <nrf52.h>
#include <nrf52_bitfields.h>
*/
#ifndef TARGET_UBITV2
#define TARGET_UBITV2
#endif //
#ifndef TARGET_NRF52
#define TARGET_NRF52
#endif
#endif // _MICROBIT_V2_

/***/


#endif // N5_UTIL
