// Duino/UBit/blink/blink.ino - Micro:Bit V1 (nrf51822) LED matrix blink control test
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Jan 2021

#include "MapLED.hpp"
#include "Common/N5/N5_HWID.hpp"

#define PIN_BTN_A 5
#define PIN_BTN_B 11

CMapLED gMap;

uint8_t iR=0, iC=0, iM=0; 

void setup (void)
{ 
  Serial.begin(115200);
  nrf5DumpHWID(Serial);
  
  pinMode(PIN_BTN_A, INPUT);
  pinMode(PIN_BTN_B, INPUT);
  gMap.init(1); // set all LEDs on
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

#if 0
int stepRow ()
{
  int r= 1;
  digitalWrite(row[iR], LOW);
  if (++iR >= sizeof(col)) { r= -iR; iR= 0; }
  digitalWrite(row[iR], HIGH);
  return(r);
} // stepRow

int stepCol (void)
{
  int r= 1;
  digitalWrite(col[iC], HIGH);
  if (++iC >= sizeof(col)) { r= -iC; iC= 0; }
  digitalWrite(col[iC], LOW);
  return(r);
} // stepCol

void rawStep (uint8_t sm)
{
  if (sm & 0x80) { gMap.setRow(1); gMap.setCol(0); }
  switch(sm & 0x3)
  {
    case 0x01 : stepRow(); 
      break;
    case 0x02 : stepCol();
      break;
    case 0x03 : if (stepRow() < 0) { stepCol(); } break;
  }
} // rawStep
#endif // old junk


void loop (void)
{
  uint8_t sm= input();
  //rawStep(sm);

  if (sm > 0)
  {
    uint8_t t0, t1;
    t0= gMap.getRCI(iR,iC);
    iR+= sm & 0x1;
    if (iR >= 5) { iR= 0; }
    iC+= (sm >> 1) & 0x1;
    if (iC >= 5) { iC= 0; }
    t1= gMap.getRCI(iR,iC);
    
    gMap.rowSwitch(t0 >> 4, t1 >> 4);
    gMap.colSwitch(t0 & 0x0F, t1 & 0x0F);
  }
  if (sm & 0x80) { Serial.println(sm,HEX); }
  else
  { // send some progress pips over host link
static uint8_t c0= 1, c1= 0;
    if (sm && (0 == --c0)) { Serial.print('.'); c0= 10; if (++c1 >= 20) { Serial.print('\n'); c1= 0; } }  
  }
  delay(50);
} // loop
