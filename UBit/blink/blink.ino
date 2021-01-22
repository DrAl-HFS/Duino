// Duino/UBit/blink/blink.ino - Micro:Bit V1 (nrf51822) initial testing
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Jan 2021

#include <nrf51.h>
#include <nrf51_bitfields.h>
//#include <nrf51_ESB.h>
//include <RH_NRF51.h>

#define PIN_BTN_A 5
#define PIN_BTN_B 11

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

// row-col multiplex 3*9 -> 5*5 (+2???)
// Arduino pin numbers
const uint8_t mapRow[]={26,27,28}; // Active high (source)
const uint8_t mapCol[]={3,4,10,23,24,25,9,7,6}; // Active low (sink)
uint8_t iR=0, iC=0, iM=0; 
// 5 * 5 matrix mapping using bytes as nybble pairs (tuples) in row:col index order
const uint8_t map55[5][5]=
{
  { 0x00, 0x13, 0x01, 0x14, 0x02, },
  { 0x23, 0x24, 0x25, 0x26, 0x27, },
  { 0x11, 0x08, 0x12, 0x28, 0x10, },
  { 0x07, 0x06, 0x05, 0x04, 0x03, },
  { 0x22, 0x16, 0x20, 0x15, 0x21 }
};

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
{ // previous & current state nybbles, upper for step (output), lower for buttons
static uint8_t levsm= 0;
  uint8_t press, sm= levsm >> 4; // get previous step
  levsm= (levsm & 0x33) << 2; // shift previous step & buttons up
  levsm|= digitalRead(PIN_BTN_A); // merge new buttons
  levsm|= digitalRead(PIN_BTN_B) << 1;
  // change state only on press transition
  press= ((levsm >> 2) ^ levsm) & levsm;
  // Buttons up/down step
  sm+= (press & 0x1);
  sm-= ((press >> 1) & 0x1);
  sm&= 0x3; // wrap
  levsm= (levsm & 0xCF) | (sm << 4); //  merge new step
  if (sm ^ (levsm>>6)) { sm|= 0x80; } // signal step change
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
  { digitalWrite(mapRow[i], v); }
} // setRow

void setCol (int v)
{
  v= (0 != v);
  for (int i=0; i<sizeof(mapCol); i++)
  { digitalWrite(mapCol[i], v); }
} // setCol

void rawStep (uint8_t sm)
{
  if (sm & 0x80) { setRow(1); setCol(0); }
  switch(sm & 0x3)
  {
    case 0x01 : stepRow(); 
      break;
    case 0x02 : stepCol();
      break;
    case 0x03 : if (stepRow() < 0) { stepCol(); } break;
  }
} // rawStep

void loop (void)
{
  uint8_t sm= input();
  //rawStep(sm);

  if (sm > 0)
  {
    uint8_t t0, t1;
    t0= map55[iR][iC];
    iR+= sm & 0x1;
    if (iR >= 5) { iR= 0; }
    iC+= (sm >> 1) & 0x1;
    if (iC >= 5) { iC= 0; }
    t1= map55[iR][iC];
    
    if ((t0 & 0xF0) != (t1 & 0xF0))
    {
       digitalWrite(mapRow[t0>>4], LOW);
       digitalWrite(mapRow[t1>>4], HIGH);
    }
    if ((t0 & 0x0F) != (t1 & 0x0F))
    {
       digitalWrite(mapCol[t0 & 0x0F], HIGH);
       digitalWrite(mapCol[t1 & 0x0F], LOW);
    }
  }
  if (sm & 0x80) { Serial.println(sm,HEX); }
  else
  { // send some progress pips over host link
static uint8_t c0= 1, c1= 0;
    if (sm && (0 == --c0)) { Serial.print('.'); c0= 10; if (++c1 >= 20) { Serial.print('\n'); c1= 0; } }  
  }
  delay(50);
} // loop
