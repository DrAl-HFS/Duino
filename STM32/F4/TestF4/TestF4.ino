// Duino/STM32/F4/TestF4.ino - Test harnes for STM32F4* hacks
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Jan 2022

#define PIN_NCS PA4 // default for SPI1 but used for builtin 32Mbit flash chip
//#define PIN_NCS PC15

#include "Common/DN_Util.hpp"
#include "Common/STM32/ST_Util.hpp"
//#include "Common/STM32/ST_Timing.hpp"
//#include "Common/STM32/ST_HWH.hpp"
#include "Common/STM32/ST_Analogue.hpp"
#include <SPI.h>
#include "Common/CW25Q.hpp"

#define DEBUG Serial1 // PA9/10 (F1xx compatible)
  
#define PIN_LED     PC13  // = LED_BUILTIN
#define PIN_BTN     PA0   // ? BTN_BUILTIN ?

#define SPI1_MOSI PA7
#define SPI1_MISO PA6 // PB4 (builtin 32Mbit flash design/manufacturing error!?), wire bridge to PA6 works for ~1MHz clock
#define SPI1_SCK  PA5
#define SPI1_NSEL PIN_NCS

#ifdef ARDUINO_ARCH_STM32F4
//define DEBUG Serial // = instance of USBSerial, same old CDC sync problems

#ifndef VARIANT_blackpill_f401 // F407
#define PIN_LED     PE0
#define PIN_BTN     PD15
#endif // f401

#endif // STM32F4


DNTimer gT(100);
CLoopBackSPI gLB;
CW25QDbg gW25QDbg(21);

bool ok=false;

#ifdef ST_ANALOGUE_HPP
ADC gADC;
#endif // ST_ANALOGUE_HPP

void bootMsg (Stream& s)
{
  s.println("\n---");
  s.print("TestF4 " __DATE__ " ");
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

#ifdef ST_ANALOGUE_HPP
  gADC.pinSetup(PA0,4);
#endif // ST_ANALOGUE_HPP
#if 0
  gLB.begin(DEBUG);
#else
  gW25QDbg.init(DEBUG);
#endif
  
} // setup

uint16_t bv=0, iter=0, dT=250, calF=0;
uint8_t chan=0;

void loop (void)
{
  if (gT.update())
  {
    uint8_t i= iter & 0x1;
    digitalWrite(PIN_LED, i);
#if 0
    gLB.test(DEBUG,i);
#else
    if (iter <= 0xF)
    {
      uint32_t bs= gW25QDbg.dumpPage(DEBUG, iter & 0xF);
      if ((0 == iter) && (2048 == bs))
      {
        char id[]= "STM32F4";
        UU32 a={0x0};
        gW25QDbg.dataWrite((uint8_t*)id, sizeof(id), a);
      }
    }
    else if (0 == (0x1F & iter)) { gW25QDbg.identify(DEBUG); }
#endif

#ifdef ST_ANALOGUE_HPP
    float a, t;

    if ((0 == gADC.n) || (iter > (gADC.ts+100)))
    {
      calF+= !gADC.calibrate(iter);
    }
    gADC.setRef(1);
    gADC.setSamplePeriod(ADC_FAST);
    if (++chan > 3) { chan=0; }
    a= gADC.readSumN(chan,4) * 0.25 * gADC.kB;
    //---
    gADC.setSamplePeriod(ADC_SLOW);
    gADC.setRef(0);
    t= gADC.readSumN(17,4) * 0.25 * ADC_VBAT_SCALE * gADC.kR;
    float tC= gADC.readTemp();

    DEBUG.print(" v="); DEBUG.print(gADC.v,5);
    DEBUG.print(" calF="); DEBUG.print(calF); // DEBUG.print(" vNRB="); DEBUG.print(gADC.vNRB); 
    DEBUG.print(" CH"); DEBUG.print(chan); DEBUG.print('='); DEBUG.print(a);
    // DEBUG.print(' '); for (int i=0; i<4; i++) { DEBUG.print(a[i]); DEBUG.print(','); }
    DEBUG.print(" CH17="); DEBUG.print(t);
    // DEBUG.print(' '); for (int i=0; i<4; i++) { DEBUG.print(t[i]); DEBUG.print(','); }
    DEBUG.print(" tC="); DEBUG.println(tC);
#endif // ST_ANALOGUE_HPP
    iter++;
  }
} // loop
