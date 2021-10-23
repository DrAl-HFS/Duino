// Duino/AVR/Test/AnTest.ino - Analogue IO test factored out
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Oct 2021

#include "Common/AVR/DA_Analogue.hpp"
#include "Common/Wave8.hpp"


CAnalogueDbg gADC;
CFastPulseDAC gDAC;
CMiniLUT8 gSin;
CSampleCtrl gDS;

ISR(ADC_vect) { gADC.event(); }

