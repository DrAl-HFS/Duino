// Duino/STM32/F4/TestF4.ino - Test harnes for STM32F4* hacks
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Jan 2022

//define HSPI    SPI2
#define PIN_NCS PA3
//#define PIN_NCS PC15

#if 0
//#include "Common/MBD/mbdDef.h" // UU16/32
#else
typedef union { uint32_t u32; uint16_t u16[2]; uint8_t u8[4]; } UU32;
#endif

#define TEST_CRC 1

//#include "Common/DN_Util.hpp"
#include "Common/dateTimeUtil.hpp"
#include "Common/STM32/ST_Util.hpp"
#include "Common/STM32/ST_Analogue.hpp"
#include "Common/STM32/ST_HWH.hpp"
#include "Common/STM32/ST_F4HWRTC.hpp"
#include <SPI.h>
#include "Common/CW25Q.hpp"


#define DEBUG Serial1 // PA9/10 (F1xx compatible)

#define PIN_LED     PC13  // = LED_BUILTIN
#define PIN_BTN     PA0   // ? BTN_BUILTIN ?

#define SPI1_MOSI PA7
#define SPI1_MISO PA6 // PB4 (builtin 32Mbit flash design/manufacturing error!?), wire bridge to PA6 works for ~21MHz clock
#define SPI1_SCK  PA5
#define SPI1_NSEL PIN_NCS // PA4 default for SPI1 but used for builtin 32Mbit flash chip


#ifdef ARDUINO_ARCH_STM32F4
//define DEBUG Serial // = instance of USBSerial, same old CDC sync problems

#ifndef VARIANT_blackpill_f401 // F407
#define PIN_LED     PE0
#define PIN_BTN     PD15
#endif // f401

#endif // STM32F4

RTCDebug gRTCDbg;
DNTimer gT(500); // 500ms -> 2Hz
CW25QDbg gW25QDbg(21);

uint16_t gIter=0;

#ifdef ST_ANALOGUE_HPP
ADCDbg gADC;
#endif // ST_ANALOGUE_HPP

#ifdef TEST_CRC
HWCRC gCRC;
#endif

void bootMsg (Stream& s)
{
  s.println("\n---");
  s.print("TestF4 " __DATE__ " ");
  s.println(__TIME__);
  dumpID(s);
} // bootMsg

//void revTest (Stream& s, uint32_t x) { s.print( x, HEX ); s.print("->"); s.println( __builtin_arm_rbit(x), HEX ); } // revTest

#define SCAN_BLOCKS 256
void hackInit (Stream& s)
{
  gADC.log(s,0x81);
  if (gW25QDbg.statf & 0x1)
  {
    uint16_t aP= 0x0000;
    UU32 a={ (uint32_t) aP << 8 };
    uint32_t i=0, t[2], n[SCAN_BLOCKS];
    
    s.println("scanEqual()"); 
    t[0]= millis();
    do
    {
      n[i]= gW25QDbg.scanEqual(a);
      a.u32+= 0x10000; // +64K
    } while (++i < SCAN_BLOCKS);
    t[1]= millis();
    i= 0;
    while (i<SCAN_BLOCKS)
    {
      for (int8_t j=0; j<16; j++) { s.print(' '); s.print(n[i++]); }
      s.println();
    }
    s.print(" dt="); s.println( t[1] - t[0] );
  }
#if 0
  if (n < 16)
  {
    s.println("Erasing...");
    gW25QDbg.dataErase(aP);
    gW25QDbg.sync();

    //char s[]="123456789ABCDEF";
    char s[]="FEDCBA987654321";
    gW25QDbg.dataWrite((uint8_t*)s, sizeof(s), a);
    gW25QDbg.sync();
  }
#endif
  //revTest(DEBUG,0xA050);
  //dumpRCCReg(DEBUG);
#if 0
  uint8_t v[]={0x11, 0x12, 0x00, 0x00};
  v[3]= bcd8Add(v+2, v[0], v[1]);
  s.print("BCD:");
  dumpHexFmt(s, v, 4);
  s.println();
#endif
} // hackInit


void hackTest (Stream& s, uint16_t i)
{
  //__QADD(); asm("QADD");
#if 0gCRC
  if (i < 1)
  {
    uint32_t *p= (uint32_t*)0x1FFFC000; // OPT 0x1FFF7800; // OTP 16*32= 512 (+16lk = 528)
    s.println("OPT:"); //"OTP:");
    for (uint8_t j=0; j<(16/4); j++)
    {
      uint32_t w= p[j];
      //if ((w > 0) && (w < 0xFFFFFFFF))
      {
        s.print('['); s.print(j); s.print("] : "); s.println(w,HEX);
      }
    }
  }
#endif
#ifdef TEST_CRC
  if (i <= 2)
  {
    testCRC(s,gCRC);
  }
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

  gW25QDbg.init(DEBUG,0);
  gRTCDbg.init(DEBUG, __TIME__, __DATE__);
  hackInit(DEBUG);

#ifdef ST_ANALOGUE_HPP
  gADC.pinSetup(PA0,ADC_TEST_NUM_CHAN); // PA0~7?
  gADC.init(DEBUG);
  gADC.clk(DEBUG);
#endif // ST_ANALOGUE_HPP
} // setup

void loop (void)
{
  if (gT.update())
  {
    uint8_t i= gIter & 0x1;
    digitalWrite(PIN_LED, i);

    if ((0x3 == (gW25QDbg.statf & 0x3)) && (gIter <= 0x10))
    {
static const uint16_t np[]={ 0x0 , (16<<10)-0x10 };
      uint16_t ap=0;
      if (gIter < 1) { ap= np[0] + gIter; } else { ap= np[1] + (gIter-1); }
      gW25QDbg.dumpPage(DEBUG, ap);
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
      //gW25QDbg.identify(DEBUG);
    }
    hackTest(DEBUG,gIter);
#ifdef ST_ANALOGUE_HPP
    gADC.setSamplePeriod(ADC_FAST);
    //gADC.test1(DEBUG,gIter);
    gADC.test2(DEBUG,gIter);
    if (2 == (gIter & 0xF)) { gADC.log(DEBUG,0x6); }
#endif // ST_ANALOGUE_HPP
    gIter++;
  }
} // loop
