// microbit1 - nrf51822 testing

#include <nrf51.h>
#include <nrf51_bitfields.h>
//#include <nrf51_ESB.h>
#include <RH_NRF51.h>

#define PIN_BTN_A 5
#define PIN_BTN_B 11


void dump (const NRF_FICR_Type *pF)
{
  int n,b;
  
  b= pF->CODEPAGESIZE;
  n= pF->CODESIZE;

  Serial.print("ubit:");
  Serial.print(b);
  Serial.print("b:");
  Serial.println(n);
} // dump

// row-col multiplex 3*9 -> 5*5
const uint8_t mapRow[]={26,27,28};
const uint8_t mapCol[]={3,4,10,23,24,25,9,7,6};
uint8_t iR=0, iC=0; 

void setup (void)
{ 
  Serial.begin(115200);
  
  dump(NRF_FICR);
  pinMode(PIN_BTN_A, INPUT);
  pinMode(PIN_BTN_B, INPUT);
  
  for (int i=0; i<sizeof(mapRow); i++)
  { 
    pinMode(mapRow[i], OUTPUT);
    digitalWrite(mapRow[i], HIGH); 
  }
  for (int i=0; i<sizeof(mapCol); i++)
  { // row-col multiplex
    pinMode(mapCol[i], OUTPUT);
    digitalWrite(mapCol[i], LOW); 
  }
  // all on
  delay(1000);
} // setup

uint8_t levsm= 0;

uint8_t input (void)
{
  uint8_t press, sm= levsm >> 6; 
  levsm= (levsm & 0x0F) << 2;
  levsm|= digitalRead(PIN_BTN_A);
  levsm|= digitalRead(PIN_BTN_B) << 1;
  // change state only on transition
  press= ((levsm >> 2) ^ levsm) & levsm;
  sm+= (press & 0x1);
  sm-= (press & 0x2) >> 1;
  sm&= 0x3;
  levsm= (levsm & 0x3F) | (sm << 6);
  if (press & 0x3) { Serial.println(sm,HEX); }
  return(sm);
} // input

void loop (void)
{ //Serial.println("blink!");
  uint8_t sm= input();
  
  digitalWrite(mapRow[iR], LOW);
  iR+= sm & 0x1;
  if (iR >= sizeof(mapRow))
  { 
    iR= 0; 
    digitalWrite(mapCol[iC], HIGH);
    iC+= (sm & 0x2) >> 1;
    if (++iC >= sizeof(mapCol)) { iC= 0; }
    digitalWrite(mapCol[iC], LOW);
  } 
  digitalWrite(mapRow[iR], HIGH);
  delay(250);
} // loop
