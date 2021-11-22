// Duino/Teensy/TestT/TestT.ino - Teensy 4.1 test fragments
// https://github.com/DrAl-HFS/Duino.git
// Licence: AGPL3
// (c) Project Contributors Oct 2021

#define SERIAL_TYPE usb_serial_class  // Teensy
#include "Common/DN_Util.hpp"
#include "Common/Teensy/TN_Timing.hpp"
#include <SPI.h>
#include "Common/CAD779x.hpp"

#define DEBUG       Serial // not USBSerial
//define DEBUG_BAUD  115200

#ifndef LED_BUILTIN
#define LED_BUILTIN 13 // =SCK
#endif

#ifndef NCS
#define NCS SS //=10
#endif

CAD779xDbg ad779x;

CMultiIntervalCounter gMIC;
void tickEvent (void) { gMIC.tickEvent(); }

const char* id (void) { return("Teensy4.1"); }

void bootMsg (Stream& s) { s.print(id()); s.println(" TestT " __DATE__ " " __TIME__); }

uint8_t pingSPI (uint8_t vO[], const uint8_t vI[], const uint8_t n, const uint8_t m=0x3)
{
  //uint8_t r=n;
  if (m & 0x2) { SPI.begin(); }
  SPI.beginTransaction(SPISettings(1E6, MSBFIRST, SPI_MODE2));
  digitalWrite(NCS,0);
  SPI.transfer(vI,vO,n);
  digitalWrite(NCS,1);
  SPI.endTransaction();
  if (m & 0x1) { SPI.end(); }
  return(n);
} // pingSPI

void setup ()
{
  noInterrupts();
#if 0
  pinMode(SCK, OUTPUT);
  pinMode(MOSI, OUTPUT);
  pinMode(MISO, INPUT);
#endif
  
  pinMode(NCS, OUTPUT);
  digitalWrite(NCS,1);
  //pinMode(LED_BUILTIN, OUTPUT);
  //digitalWrite(LED_BUILTIN,0);
  interrupts();

  if (beginSync(DEBUG)) { bootMsg(DEBUG); } //.begin(DEBUG_BAUD);
/*
  DEBUG.print("NCS="); DEBUG.println(NCS);
  DEBUG.print("SCK="); DEBUG.println(SCK);
  DEBUG.print("MOSI="); DEBUG.println(MOSI);
  DEBUG.print("MISO="); DEBUG.println(MISO);
*/
  gMIC.init(tickEvent,1000); // 1Hz
  ad779x.reset();
  ad779x.test(DEBUG);
} // setup

uint8_t idx= 0;

void loop ()
{
  const uint8_t sig= gMIC.update();
  if (sig > 0) { ad779x.sample(); }
  if (sig > 0x1)
  {
    if (ad779x.ready()) { ad779x.report(DEBUG); } else { ad779x.test(DEBUG); }
    const AD779x::Chan cm[]= {AD779x::AVDD, AD779x::THERM};
    ad779x.setChan( cm[ idx & 0x1 ] );
    /*
    uint8_t vI[3]={0b01100000,0x01,0x00}, vO[3], r, m=0x3;
    //v= 0xF0 | (idx & 0x0F);
    r= pingSPI(vO, vI, 2);
    for (int i=0; i<r; i++)
    {
      DEBUG.print(vO[i],HEX); DEBUG.print(',');
    } DEBUG.println();// DEBUG.print(' ');
    */
    //if (0 == (idx & 0x0F)) { DEBUG.print('.'); }
    //if (0 == idx) { DEBUG.println(); }
    //digitalWrite(LED_BUILTIN, idx & 0x1);
    //DEBUG.println('.');
    idx++;
  }
} // loop
