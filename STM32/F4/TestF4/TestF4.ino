// Duino/STM32/F4/TestF4.ino - Test harnes for STM32F4* hacks
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Jan 2022

#define PIN_NCS PA4 // default for SPI1 but used for builtin 32Mbit flash chip
//#define PIN_NCS PC15

#if 0
//#include "Common/MBD/mbdDef.h" // UU16/32
#else
typedef union { uint32_t u32; uint16_t u16[2]; uint8_t u8[4]; } UU32;
#endif

#include "Common/DN_Util.hpp"
#include "Common/STM32/ST_Util.hpp"
//#include "Common/STM32/ST_Analogue.hpp"
//include "Common/STM32/ST_HWH.hpp"
#include "Common/STM32/ST_F4HWRTC.hpp"
//#include <RTClock.h>
#include <SPI.h>
#include "Common/CW25Q.hpp"


#define DEBUG Serial1 // PA9/10 (F1xx compatible)

#define PIN_LED     PC13  // = LED_BUILTIN
#define PIN_BTN     PA0   // ? BTN_BUILTIN ?

#define SPI1_MOSI PA7
#define SPI1_MISO PA6 // PB4 (builtin 32Mbit flash design/manufacturing error!?), wire bridge to PA6 works for ~21MHz clock
#define SPI1_SCK  PA5
#define SPI1_NSEL PIN_NCS

#ifdef ARDUINO_ARCH_STM32F4
//define DEBUG Serial // = instance of USBSerial, same old CDC sync problems

#ifndef VARIANT_blackpill_f401 // F407
#define PIN_LED     PE0
#define PIN_BTN     PD15
#endif // f401

#endif // STM32F4

RTCDebug gRTCDbg;
//RTClock gRTC;
DNTimer gT(100);
CW25QDbg gW25QDbg(21);

uint16_t gIter=0;

#ifdef ST_ANALOGUE_HPP
ADC gADC;
uint16_t calF=0;
uint8_t chan=0;

void testADC (Stream& s, ADC& adc)
{
    uint32_t m[4];
    float a, t;

    if ((0 == adc.n) || (gIter > (adc.ts+100)))
    {
      calF+= !adc.calibrate(gIter);
    }
    adc.setRef(1);
    adc.setSamplePeriod(ADC_FAST);
    if (++chan > 3) { chan=0; }
    m[0]= millis();
    a= adc.readSum16(chan,10000) * (1.0/10000) * adc.kB;
    m[1]= millis();
    //---
    adc.setSamplePeriod(ADC_SLOW);
    adc.setRef(0);
    m[2]= millis();
    t= adc.readSumN(17,250) * (1.0/250) * ADC_VBAT_SCALE * adc.kR;
    m[3]= millis();
    float tC= adc.readTemp();

    s.print(" v="); s.print(adc.v,5);
    s.print(" calF="); s.print(calF); // s.print(" vNRB="); s.print(adc.vNRB); 
    s.print(" CH"); s.print(chan); s.print('='); s.print(a);
    // s.print(' '); for (int i=0; i<4; i++) { s.print(a[i]); s.print(','); }
    s.print(" CH17="); s.print(t);
    // s.print(' '); for (int i=0; i<4; i++) { s.print(t[i]); s.print(','); }
    s.print(" tC="); s.println(tC);

    for (uint8_t i=0; i<4; i++) { s.print(m[i]); s.print(','); }
    s.println();
} // testADC

#endif // ST_ANALOGUE_HPP

void bootMsg (Stream& s)
{
  s.println("\n---");
  s.print("TestF4 " __DATE__ " ");
  s.println(__TIME__);
  dumpID(s);
} // bootMsg

//void revTest (Stream& s, uint32_t x) { s.print( x, HEX ); s.print("->"); s.println( __builtin_arm_rbit(x), HEX ); } // revTest

void hackInit (Stream& s)
{
  //revTest(DEBUG,0xA050);
  //crcEnable();
  //dumpRCCReg(DEBUG);
  //gRTC.setTime(1643392338); useless
  //gRTC.begin();
#if 0
  uint8_t v[]={0x11, 0x12, 0x00, 0x00};
  v[3]= bcd8Add(v+2, v[0], v[1]);
  s.print("BCD:");
  dumpHexFmt(s, v, 4);
  s.println();
#endif
} // hackInit


void hackTest (Stream& s)
{
#if 0
  uint32_t v[]={0x01234567,0x89abcdef};
  uint32_t nc= crc(v,sizeof(v)/sizeof(v[0]));
  DEBUG.print("crc="); DEBUG.println(nc);
#endif
  //for (uint8_t i=1; i<=5; i++) { dumpTimReg(DEBUG, i); }
} // hackTest

void setup (void)
{
  noInterrupts();

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, 0);

  pinMode(PIN_BTN, INPUT);

  interrupts();
  if (beginSync(DEBUG)) { bootMsg(DEBUG); }

#ifdef ST_ANALOGUE_HPP
  gADC.pinSetup(PA0,4); // PA0~3
#endif // ST_ANALOGUE_HPP

  gW25QDbg.init(DEBUG);
  hackInit(DEBUG);
  gRTCDbg.init(DEBUG);
} // setup

void loop (void)
{
  if (gT.update())
  {
    uint8_t i= gIter & 0x1;
    digitalWrite(PIN_LED, i);

    if (gIter <= 0x1)
    {
static const uint16_t np[]={ 0x0 , (16<<10)-1 };
      //uint32_t bs=
      gW25QDbg.dumpPage(DEBUG, np[gIter]);
      /*
      if ((0 == gIter) && (2048 == bs))
      {
        char id[]= "STM32F4";
        UU32 a={0x0};
        gW25QDbg.dataWrite((uint8_t*)id, sizeof(id), a);
      }
      */
    }
    else if (0 == (0x1F & gIter))
    {
      gRTCDbg.dump(DEBUG);
      gW25QDbg.identify(DEBUG);
    }

#ifdef ST_ANALOGUE_HPP
    testADC(DEBUG,gADC);
#endif // ST_ANALOGUE_HPP
    gIter++;
  }
} // loop
