// Duino/STM32/F4/BlinkF4.ino - STM32F4* basic test
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Mar 2020 - Sept 2021

#define PIN_NCS PA4 // default for SPI1 but used for builtin 32Mbit flash chip
//#define PIN_NCS PC15

#include "Common/DN_Util.hpp"
#include "Common/STM32/ST_Util.hpp"

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

uint32_t gIter=0;
DNTimer gT(100); // 100ms -> 10Hz

void bootMsg (Stream& s)
{
  s.println("\n---");
  s.print("BlinkF4 " __DATE__ " ");
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
  //gIter= 0;
} // setup

void log (Stream& s, uint32_t i)
{
static const char sep[2]={' ','\n'};
  uint8_t iS= (0 == (i & 0xF));
  s.print(i); s.print( sep[iS] );
} // log

void loop (void)
{
  if (gT.update())
  {
    ++gIter;
    digitalWrite(PIN_LED, gIter & 0x1);
    log(DEBUG,gIter);
  }
} // loop
