// Duino/UBit/radio/radio.ino - Micro:Bit V1 (nrf51822) radio test
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Jan 2021

#define RADIO_MODE_ESB
#include <RH_NRF51.h>
#include "Common/M0_Util.hpp"
#include "Common/N5/N5_HWID.hpp"
#include "Common/N5/N5_Timing.hpp"

#define PIN_BTN_A 5
#define PIN_BTN_B 11

/*** GLOBALS ***/

CClock gClock;

RH_NRF51 gRF;

int gLogIvl= 100;
int8_t rmm[2]={127,-128}, gChan=2;

uint32_t gNextTick;


/*** ISR ***/

// NB - 'duino "magic" name linking requires undecorated symbol
extern "C" {

void TIMER2_IRQHandler (void)
{
	if (0 != NRF_TIMER2->EVENTS_COMPARE[0])
  {
    //if (NRF_TIMER2->INTENSET & TIMER_INTENSET_COMPARE0_Msk)
    //{ NRF_TIMER2->INTENCLR= TIMER_INTENSET_COMPARE0_Msk; } // Clear interrupt = single shot
		NRF_TIMER2->EVENTS_COMPARE[0]= 0; // Clear event (SHORTS resets count)
    while (0 != NRF_TIMER2->EVENTS_COMPARE[0]); // spin sync
    gClock.tickEvent();
  }
} // TIMER2_IRQHandler

void RTC0_IRQHandler (void)
{
	if (0 != NRF_RTC0->EVENTS_OVRFLW)
  {
    NRF_RTC0->EVENTS_OVRFLW= 0; // clear & spin sync
    while (0 != NRF_RTC0->EVENTS_OVRFLW);
    gClock.ofloEvent(); // update offset
  }
} // RTC0_IRQHandler

} // extern "C"

/***/

void setup (void)
{ 
  gClock.init();

  Serial.begin(115200); // ? 230400); //
  
  n5DumpHWID(Serial);
  pinMode(PIN_BTN_A, INPUT);
  pinMode(PIN_BTN_B, INPUT);
  
  gRF.init();
#if 1
  uint8_t hms[3];
  hms[0]= bcd4ToU8(bcd4FromChar(__TIME__+0,2),2);
  hms[1]= bcd4ToU8(bcd4FromChar(__TIME__+3,2),2);
  hms[2]= bcd4ToU8(bcd4FromChar(__TIME__+5,2),2);
  gClock.setHMS(hms);
  gNextTick= gClock.offset(10);
#endif
  gClock.start();
} // setup

void loop (void)
{
  char b[32];
  uint8_t n=7;
  b[n]= 0;
  
  if (gClock.interval(gNextTick))
  { // much more accurate than delay()
    gNextTick+= 10;
    // Report time immediately to prevent mismatch
    if (--gLogIvl <= 0) { gClock.print(Serial); }

    if (gRF.recv((uint8_t*)b,&n))
    {
      gRF.setModeRx();
      Serial.print(b);
      Serial.println("<-RF");
    }
    else
    {
      NRF_RADIO->TASKS_RSSISTART= 1;
      while (0 == NRF_RADIO->EVENTS_RSSIEND); // spin
      uint8_t r= NRF_RADIO->RSSISAMPLE;
      r&= 0x7F;
      rmm[0]= min(rmm[0],r);
      rmm[1]= max(rmm[1],r);
    }
    if (gLogIvl <= 0)
    { // too much for single 10ms slot ???
      gLogIvl= 100;
      Serial.print(" CH"); Serial.print(gChan); Serial.print(" : ");
      Serial.print(rmm[0]); Serial.print(','); Serial.println(rmm[1]);// Serial.print(" | ");
      gChan= (gChan+1) & 0xF;
      gRF.setChannel(gChan);
      rmm[0]= 127; rmm[1]= -128;
    }
  }
} // loop
