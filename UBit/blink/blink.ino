// Duino/UBit/blink/blink.ino - Micro:Bit V1 (nrf51822) initial testing
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Jan 2021

#include <nrf51.h>
#include <nrf51_bitfields.h>
//#include <nrf51_ESB.h>
#include <RH_NRF51.h>

#define PIN_BTN_A 5
#define PIN_BTN_B 11

// Howto read hardware registers...
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
const uint8_t mapRow[]={26,27,28}; // Active high (source)
const uint8_t mapCol[]={3,4,10,23,24,25,9,7,6}; // Active low (sink)
uint8_t iR=0, iC=0; 

void setup (void)
{ 
  Serial.begin(115200);
  
  dump(NRF_FICR);
  pinMode(PIN_BTN_A, INPUT);
  pinMode(PIN_BTN_B, INPUT);
  
  // set all on
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
} // setup

// Read buttons, get transitions, set state
uint8_t input (void)
{
static uint8_t levsm= 0;
  uint8_t press, sm= levsm >> 6; 
  levsm= (levsm & 0x33) << 2;
  levsm|= digitalRead(PIN_BTN_A);
  levsm|= digitalRead(PIN_BTN_B) << 1;
  // change state only on press transition
  press= ((levsm >> 2) ^ levsm) & levsm;
  sm+= (press & 0x1);
  sm-= (press & 0x2) >> 1;
  sm&= 0x3;
  levsm= (levsm & 0xCF) | (sm << 4);
  if (press & 0x3) { Serial.println(sm,HEX); }
  if (sm ^ (levsm>>6)) { sm|= 0x80; }
  return(sm);
} // input

int stepRow ()
{
  int r= 1;
  digitalWrite(mapRow[iR], LOW);
  if (++iR >= sizeof(mapCol)) { r= -iR; iR= 0; }
  digitalWrite(mapRow[iR], HIGH);
  return(r);
} // stepRow

int stepCol (void)
{
  int r= 1;
  digitalWrite(mapCol[iC], HIGH);
  if (++iC >= sizeof(mapCol)) { r= -iC; iC= 0; }
  digitalWrite(mapCol[iC], LOW);
  return(r);
} // stepCol

void setRow (int v)
{
  v= (0 != v);
  for (int i=0; i<sizeof(mapRow); i++)
  { digitalWrite(mapRow[i], HIGH); }
} // setRow

void setCol (int v)
{
  v= (0 != v);
  for (int i=0; i<sizeof(mapCol); i++)
  { digitalWrite(mapCol[i], v); }
} // setCol

void loop (void)
{ //Serial.println("blink!");
  uint8_t sm= input();
  
  if (sm & 0x80) { setRow(1); setCol(0); }
  switch(sm & 0x3)
  {
    case 0x01 : stepRow(); 
      break;
    case 0x02 : stepCol();
      break;
    case 0x03 : if (stepRow() < 0) { stepCol(); } break;
  } 
  
  delay(100);
} // loop
