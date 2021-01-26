// Duino/UBit/radio/radio.ino - Micro:Bit V1 (nrf51822) radio test
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Jan 2021

#define RADIO_MODE_ESB
#include <nrf51.h>
#include <nrf51_bitfields.h>
#include <RH_NRF51.h>
//include "MapLED.hpp"

#define PIN_BTN_A 5
#define PIN_BTN_B 11

/*** ISR ***/
volatile uint16_t gTick= 0, gTock= 0;

extern "C" {
// NB - 'duino "magic" name linking requires undecorated symbol
void TIMER2_IRQHandler (void)
{
	if (0 != NRF_TIMER2->EVENTS_COMPARE[0])
  {
    //if (NRF_TIMER2->INTENSET & TIMER_INTENSET_COMPARE0_Msk)
    //{ NRF_TIMER2->INTENCLR= TIMER_INTENSET_COMPARE0_Msk; } // Clear interrupt = single shot
		NRF_TIMER2->EVENTS_COMPARE[0]= 0; // Clear event (SHORTS resets count)
    while (0 != NRF_TIMER2->EVENTS_COMPARE[0]); // spin sync
    if (++gTick >= 60000) { ++gTock; gTick-= 60000; }
  }
} // IRQ

} // extern "C"

void timerSetup (void)
{		
  NRF_TIMER2->MODE = TIMER_MODE_MODE_Timer;
  NRF_TIMER2->PRESCALER= 4;   // 16 clocks -> micro-second tick
  NRF_TIMER2->TASKS_CLEAR= 1; // clean reset
	NRF_TIMER2->BITMODE= TIMER_BITMODE_BITMODE_16Bit; // 24, 32 unnecessary
	NRF_TIMER2->CC[0]= 1000;   // 1ms
		
  // Enable auto clear of count & interrupt for CC[0]
  NRF_TIMER2->SHORTS=   TIMER_SHORTS_COMPARE0_CLEAR_Enabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos;
  NRF_TIMER2->INTENSET= (TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos);
  // | (TIMER_INTENSET_COMPARE1_Enabled << TIMER_INTENSET_COMPARE1_Pos);
  NVIC_EnableIRQ(TIMER2_IRQn);
} // timerSetup

void timerGo (void) { NRF_TIMER2->TASKS_START= 1; }

// Howto read hardware registers...
void dump (const NRF_FICR_Type *pF)
{
  int n,b;
  
  b= pF->CODEPAGESIZE;
  n= pF->CODESIZE;

  Serial.print("\nubit:P=");
  Serial.print(b);
  Serial.print("byte:N=");
  Serial.println(n);
} // dump

RH_NRF51 gRF;

void setup (void)
{ 
  timerSetup();
  Serial.begin(115200);
  
  dump(NRF_FICR);
  pinMode(PIN_BTN_A, INPUT);
  pinMode(PIN_BTN_B, INPUT);
  
  gRF.init();
  
  timerGo();
} // setup

int gI= 0, gJ= 0;
int8_t rmm[2]={127,-128}, gChan=2;

volatile uint16_t gNextTick;

void loop (void)
{
  char b[32];
  uint8_t n=7;
  b[n]= 0;
  
  if (gTick >= gNextTick)
  { // much more accurate than delay() but extra +10ms per second ??
    gNextTick+= 10;
    if (gNextTick >= 60000) { gNextTick-= 60000; }
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
      if (++gI > 100)
      { // too much for single 10ms slot ???
        Serial.print(gTock); Serial.print(':'); Serial.print(gTick); Serial.print(' ');
        Serial.print("C"); Serial.print(gChan); Serial.print(" : ");
        Serial.print(rmm[0]); Serial.print(','); Serial.println(rmm[1]);// Serial.print(" | ");
        gI= 0;
        //if (++gJ > 10) { gJ=0; }
        gChan= (gChan+1) & 0xF;
        gRF.setChannel(gChan);
        rmm[0]= 127; rmm[1]= -128;
      }
    }
  }
  //delay(10);
} // loop
