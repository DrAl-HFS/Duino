// Duino/STM32/F4/BlinkF4.ino - STM32F4* basic test
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Mar 2020 - Sept 2021

#include "Common/STM32/ST_Util.hpp"
#include "Common/STM32/ST_Timing.hpp"
#include "Common/STM32/ST_Analogue.hpp"

#define DEBUG Serial1 // PA9/10 (F1xx compatible)
  
#define PIN_LED     PC13  // = LED_BUILTIN
#define PIN_BTN     PA0   // ? BTN_BUILTIN ?

#ifdef ARDUINO_ARCH_STM32F4
//define DEBUG Serial // = instance of USBSerial, same old CDC sync problems

#ifndef VARIANT_blackpill_f401 // F407
#define PIN_LED     PE0
#define PIN_BTN     PD15
#endif // f401

#endif // STM32F4


ADC gADC;
CTimer gT;
void oflo (void) { gT.nextIvl(); }


void bootMsg (Stream& s)
{
  DEBUG.print("BlinkF4 " __DATE__ " ");
  s.println(__TIME__);
  dumpID(s);
} // bootMsg

void setup (void)
{
  noInterrupts();

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, 0);
  
  pinMode(PIN_BTN, INPUT);

  interrupts();
  if (beginSync(DEBUG)) { bootMsg(DEBUG); }
  
  gT.start(oflo);
  //gT.dbgPrint(DEBUG);
  gADC.pinSetup(PA1,4);
} // setup

uint16_t bv=0, iter=0, lastD=0, dT=250, calF=0;
uint8_t chan=0;

void loop (void)
{
  uint16_t d= gT.diff();
  if (d > 0)
  {
#if 0
    if (d > lastD)
    {
      lastD= d;
      bv= (bv<<1) | digitalRead(PIN_BTN);
      if (0xFF00 == bv) // inverted input
      {
        dT*= 2;
        if (dT > 400) { dT= 100; }
        DEBUG.print("dT="); DEBUG.println(dT);
      }
    }
#endif
    if (d >= dT)
    {
      float a, t;
      
      gT.retire(dT); lastD= 0;
      digitalWrite(PIN_LED, ++iter & 0x1);

      if ((0 == gADC.n) || (iter > (gADC.ts+100)))
      {
        calF+= !gADC.calibrate(iter);
      }
      gADC.setRef(1);
      gADC.setSamplePeriod(ADC_FAST);
      if (++chan > 4) { chan=1; }
      a= gADC.readSumN(chan,4) * 0.25 * gADC.kB;
      //---
      gADC.setSamplePeriod(ADC_SLOW);
      gADC.setRef(0);
      t= gADC.readSumN(17,4) * 0.25 * ADC_VBAT_SCALE * gADC.kR;
      float tC= gADC.readTemp();
      //---

      DEBUG.print('T'); DEBUG.print(gT.swTickVal()); DEBUG.print(" v="); DEBUG.print(VCal::gADC.v,5);
      DEBUG.print(" calF="); DEBUG.print(calF); // DEBUG.print(" vNRB="); DEBUG.print(gADC.vNRB); 
      DEBUG.print(" CH"); DEBUG.print(chan); DEBUG.print('='); DEBUG.print(a);
      // DEBUG.print(' '); for (int i=0; i<4; i++) { DEBUG.print(a[i]); DEBUG.print(','); }
      DEBUG.print(" CH17="); DEBUG.print(t);
      // DEBUG.print(' '); for (int i=0; i<4; i++) { DEBUG.print(t[i]); DEBUG.print(','); }
      DEBUG.print(" tC="); DEBUG.println(tC);
    }
  }
  //delay(dT);
} // loop
