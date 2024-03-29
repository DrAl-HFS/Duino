// Duino/UBit/BlinkN5/blink.ino - Micro:Bit LED matrix blink control test
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Jan 20 - June 21

#include "Common/dateTimeUtil.h"
#include "Common/N5/N5_HWID.hpp"
#include "Common/N5/N5_ClockInstance.hpp"
#include "MapLED.hpp"
#include "Buttons.hpp"
#ifdef TARGET_UBITV2
#include "Audio.hpp"
#endif

#define DEBUG Serial

// Extend with virtual "double-press" button using timed lockout
class BlinkInput : public CUBitButtons
{
public:
  uint8_t state;
  
  BlinkInput (void) : state{0} { ; }
  
  uint8_t update (void)
  {
    uint8_t t= 0, ev= 0;
    CUBitButtons::update();
    if (state >= 0x20) // locked
    {
      t= getTimeAB(0);
      if (t >= 5) { state= 0x10 | (t & 0x0F); } // begin unlock
    }
    else
    {
      ev= getPress();
      if (state >= 0x10) { state-= 0x10; } // unlock transition
      else
      { // check for double-press lock
        t= getTimeAB(1);
        if (t >= 10) { state= 0x20 | (t & 0x0F); ev|= 0x04; } // flip & lock
      }
    }
    return(ev);
  }
}; // BlinkInput

CSpeaker gSpkr;
CMapLED gMap;
BlinkInput gBI;

uint8_t iR=0, iC=0;
uint8_t mode=0x03, cycleA= 0, cycleB= 0; 

void bootMsg (Stream& s)
{
  s.println("\n---");
  s.print("Blink N5 " __DATE__ " ");
  s.println(__TIME__);
  nrf5DumpHWID(s);
} // bootMsg

void setup (void)
{ 
  DEBUG.begin(115200);
  bootMsg(DEBUG);
  
  gSpkr.init();
  //analogWrite(SPKR_PIN, 0x100);

  gMap.init(0); // set all LEDs off ?
  
  gBI.init();

  gClock.init(50); // 50ms -> 20Hz
  uint8_t hms[3]; // TODO: factor
  bcd4FromTimeA(hms,__TIME__);
  gClock.setHMS(hms);

  gClock.start();
} // setup

#if 0 // broken..
  if (mode & 0x60)
  {
    gMap.setRow(0x1F);
    if (mode & 0x20) { gMap.setCol(0x1FF); mode&= ~0x20;  } // all off
    else { gMap.setCol(0x000); } // all on
  }
  else
#endif

void loop (void)
{
  if (gClock.elapsed())
  {
    uint8_t ev= gBI.update();
    if (ev & 0x04)
    {
      uint8_t m0= mode;
      mode+= 0x10;
      if ((mode & 0xF0) > 0x10) { mode&= 0x0F; }
      Serial.print("M:x"); Serial.print(m0,HEX); Serial.print("->x"); Serial.println(mode,HEX);
    }
    if (0x10 == (mode & 0xF0))
    { // continuous
      mode^= ev & 0x3;
      ev= mode & 0x3;
    }
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
    if (++cycleA < 20) { digitalWrite(MIC_LED_PIN, 0); }
    else
    {
      cycleA= 0;
      digitalWrite(MIC_LED_PIN, 1); gSpkr.clear();
      if (++cycleB >= 60) { gSpkr.pulse(128); cycleB-= 60; }
      else switch(cycleB & 0x1)
      {
        case 0 : gSpkr.click(); break; // tick
        case 1 : gSpkr.tone(); break; // tock
      }

      gClock.print(Serial,'\n');
    }  
  }
  gSpkr.update(gClock.getTick());
} // loop
