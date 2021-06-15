// Duino/UBit/RadioN5/RadioN5.ino - Micro:Bit V1 (nrf51822) radio test
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Jan 2021

// Ref:
// infocenter.nordicsemi.com/topic/com.nordic.infocenter.sdk5.v15.2.0/esb_users_guide.html
// github.com/NordicPlayground/nrf51-micro-esb/Common/micro_esb.c
//SDK v15 #define NRF_ESB_LEGACY

#include "Common/N5/N5_HWID.hpp"
#include "Common/N5/N5_ClockInstance.hpp" // changed from N5_Timing
#include "N5_RF.hpp"

#define PIN_BTN_A 5
#define PIN_BTN_B 11

#define N5_CHAN_BASE 35
#define N5_CHAN_COUNT 10
#define N5_CHAN_STEP 2


/*** GLOBALS ***/

CRFScan gScan(N5_CHAN_BASE,N5_CHAN_COUNT,N5_CHAN_STEP);

RH_NRF51 gRF;

int gLogIvl= 100;


/***/

void setup (void)
{ 
  gClock.init();

  Serial.begin(115200); // UART->USB link - the usual limitations apply...
  
  nrf5DumpHWID(Serial);
  pinMode(PIN_BTN_A, INPUT);
  pinMode(PIN_BTN_B, INPUT);
  
  gRF.init();
  // gRF.printRegisters(); ???
  
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
      //st[iS++]= getState();
      gClock.tickCapture(1);
      gScan.scan(N5_CHAN_COUNT);
      gClock.tickCapture(2);
      //st[iS++]= getState();
    }
    if (gLogIvl <= 0)
    {
      gLogIvl= 100;
#if 0
      Serial.print(" A"); Serial.print(NRF_RADIO->RXADDRESSES,HEX);
      Serial.print(" "); Serial.print(NRF_RADIO->BASE0,HEX);
      Serial.print(" "); Serial.print(NRF_RADIO->PREFIX0,HEX);
      Serial.print(" S["); Serial.print(iS); Serial.print(']');
      for (int8_t i=0; i<iS; i++) {  Serial.print(st[i]); Serial.print(','); } 
      //Serial.print(NRF_CLOCK->HFCLKSTAT,HEX);
      //Serial.print(NRF_CLOCK->LFCLKSTAT,HEX);
#endif
      gScan.print(Serial,N5_CHAN_COUNT);
      Serial.print(" dt="); gClock.printTCI(Serial); Serial.println("us");
    }
  }
} // loop
