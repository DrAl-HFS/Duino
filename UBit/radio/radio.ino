// Duino/UBit/radio/radio.ino - Micro:Bit V1 (nrf51822) radio test
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Jan 2021

// Ref:
// infocenter.nordicsemi.com/topic/com.nordic.infocenter.sdk5.v15.2.0/esb_users_guide.html
// github.com/NordicPlayground/nrf51-micro-esb/Common/micro_esb.c
//SDK v15 #define NRF_ESB_LEGACY

#define RADIO_MODE_ESB
#include <RH_NRF51.h>
#include "N5_RF.hpp"
#include "Common/M0_Util.hpp"
#include "Common/N5/N5_HWID.hpp"
#include "Common/N5/N5_Timing.hpp"

#define PIN_BTN_A 5
#define PIN_BTN_B 11

/*** GLOBALS ***/

CClock gClock;
CRFScan gScan;

RH_NRF51 gRF;

int gLogIvl= 100;
uint8_t gBaseChan=44, gChanOffset=0, gChan=0;


/*** ISR ***/

// NB - 'duino "magic" name linking requires undecorated symbol
extern "C" {

void TIMER2_IRQHandler (void)
{
  NRF_TIMER_Type *pT=NRF_TIMER2;
	if (0 != pT->EVENTS_COMPARE[3])
  {
		pT->EVENTS_COMPARE[3]= 0; // Clear event (SHORTS resets count)
    while (0 != pT->EVENTS_COMPARE[3]); // spin sync
    gClock.tickEvent();
  }
} // TIMER2_IRQHandler

void RTC0_IRQHandler (void)
{
	if (0 != NRF_RTC0->EVENTS_OVRFLW)
  {
    NRF_RTC0->EVENTS_OVRFLW= 0; // clear & spin sync
    while (0 != NRF_RTC0->EVENTS_OVRFLW);
    gClock.rtcOvfloEvent(); // update offset
  }
} // RTC0_IRQHandler

} // extern "C"

/***/

void setup (void)
{ 
  gClock.init();

  Serial.begin(115200); // UART->USB link - the usual limitations apply...
  
  n51DumpHWID(Serial);
  pinMode(PIN_BTN_A, INPUT);
  pinMode(PIN_BTN_B, INPUT);
  
  gRF.init();

  // Set clock from build time
  uint8_t hms[3];
  hms[0]= bcd4ToU8(bcd4FromChar(__TIME__+0,2),2);
  hms[1]= bcd4ToU8(bcd4FromChar(__TIME__+3,2),2);
  hms[2]= bcd4ToU8(bcd4FromChar(__TIME__+5,2),2);
  gClock.setHMS(hms);

  gClock.start();
} // setup

void loop (void)
{
  char b[32];
  int8_t st[4], iS=0;
  uint8_t n=7;
  b[n]= 0;
  
  if (gClock.elapsed())
  {
    // Report time immediately to prevent mismatch
    if (--gLogIvl <= 0) { gClock.print(Serial); }

    gChan= gBaseChan + gChanOffset;
    gRF.setChannel(45);
    st[iS++]= getState();
    if (gRF.recv((uint8_t*)b,&n))
    {
      st[iS++]= getState();
      Serial.print(b);
      Serial.println("<-RF");
    }
    else
    {
      st[iS++]= getState();
      gClock.tickCapture(1);
      gRF.setChannel(gChan);
      st[iS++]= getState();
      gScan.scan(gChanOffset);
      gClock.tickCapture(2);
      st[iS++]= getState();
    }
    if (gLogIvl <= 0)
    {
      gLogIvl= 100;
      Serial.print(" A"); Serial.print(NRF_RADIO->RXADDRESSES,HEX);
      Serial.print(" "); Serial.print(NRF_RADIO->BASE0,HEX);
      Serial.print(" "); Serial.print(NRF_RADIO->PREFIX0,HEX);
      Serial.print(" S["); Serial.print(iS); Serial.print(']');
      for (int8_t i=0; i<iS; i++) {  Serial.print(st[i]); Serial.print(','); } 
      //Serial.print(NRF_CLOCK->HFCLKSTAT,HEX);
      //Serial.print(NRF_CLOCK->LFCLKSTAT,HEX);
      Serial.print(" CH"); Serial.print(gChan);
      gScan.print(gChanOffset,Serial);
      Serial.print(" dt="); Serial.println(gClock.tickCaptureInterval(1,2));
      gChanOffset= 0x3 & (gChanOffset+1);
    }
  }
} // loop
