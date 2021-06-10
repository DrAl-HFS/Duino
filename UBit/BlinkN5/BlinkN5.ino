// Duino/UBit/BlinkN5/blink.ino - Micro:Bit V1 (nrf51822) LED matrix blink control test
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Jan 2021

#include "Common/N5/N5_HWID.hpp"
#include "MapLED.hpp"
#include "Buttons.hpp"

CMapLED gMap;
CUBitButtons gB;

uint8_t iR=0, iC=0;
uint8_t mode=0x00; 

void setup (void)
{ 
  Serial.begin(115200);
  nrf5DumpHWID(Serial);

  gMap.init(0); // set all LEDs off ?
  
  gB.init();
} // setup

void loop (void)
{
  uint8_t t, ev; 

  gB.update();
  ev= gB.getPress();
  if (mode & 0x40) // locked
  {
    t= gB.getTimeAB(0);
    if (t >= 2) { mode&= ~0x40; ev|= 0x40; } // unlock
  }
  else
  {
    t= gB.getTimeAB(1);
    if (t >= 10) { mode^= 0x80; mode|= 0x40; ev|= 0x80; } // flip & lock
  }
  if (mode & 0x80)
  { // continuous
    mode^= ev & 0x3;
    ev= mode & 0x3;
  }
  if (ev & 0xF0) { Serial.print("M=x"); Serial.print(mode,HEX); Serial.print(" t="); Serial.println(t);}
  if (ev & 0x3)
  {
    uint8_t t0, t1;
    t0= gMap.getRCI(iR,iC);
    iR+= ev & 0x1;
    if (iR >= 5) { iR= 0; }
    iC+= (ev >> 1) & 0x1;
    if (iC >= 5) { iC= 0; }
    t1= gMap.getRCI(iR,iC);
    
    gMap.rowSwitch(t0 >> 4, t1 >> 4);
    gMap.colSwitch(t0 & 0x0F, t1 & 0x0F);
  }

  delay(50);
} // loop
