// Duino/UBit/blink/blink.ino - Micro:Bit V1 (nrf51822) LED matrix blink control test
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Jan 2021

#include "Common/N5/N5_HWID.hpp"
#include "MapLED.hpp"
#include "Buttons.hpp"

CMapLED gMap;
CUBitButtons gB;

uint8_t iR=0, iC=0, iM=0, mm=0; 

void setup (void)
{ 
  Serial.begin(115200);
  nrf5DumpHWID(Serial);

  gMap.init(0); // set all LEDs off ?
  
  gB.init();
  //pinMode(PIN_BTN_A, INPUT);
  //pinMode(PIN_BTN_B, INPUT);
} // setup

// Read buttons, get transitions, set state
uint8_t input (void)
{ // previous & current state nybbles, upper for step (output), lower for buttons
#if 0
static uint8_t levsm= 0;
  uint8_t press, sm= levsm >> 4; // get previous step
  levsm= (levsm & 0x33) << 2; // shift previous step & buttons up
  levsm|= digitalRead(PIN_BTN_A); // merge new buttons
  levsm|= digitalRead(PIN_BTN_B) << 1;
  // change state only on press transition
  press= ((levsm >> 2) ^ levsm) & levsm;
  // Buttons up/down step
  if (0x3 == press) { sm|= 0x40; }
  else
  {
    sm+= (press & 0x1);
    sm-= ((press >> 1) & 0x1);
    sm&= 0x3; // wrap
  }
  levsm= (levsm & 0xCF) | (sm << 4); //  merge new step
  if (sm ^ (levsm>>6)) { sm|= 0x80; } // signal step change
  return(sm);
#else
  gB.update();
  return( (gB.a.getPress() << 6) | ( gB.b.getPress() << 7) | gB.a.getState() | (gB.b.getState() << 1) );
#endif
} // input

#if 1
int stepRow ()
{
  int r= 1;
  int8_t t0, t1;
  
  t0= row[iR];
  if (++iR >= sizeof(col)) { r= -iR; iR= 0; }
  t1= row[iR];
  digitalWrite(t0, LOW);
  digitalWrite(t1, HIGH);
  Serial.print('R'); Serial.print(t0,HEX); Serial.print("->"); Serial.println(t1,HEX); 
  return(r);
} // stepRow

int stepCol (void)
{
  int r= 1;
  int8_t t0, t1;
  t0= row[iC];
  if (++iC >= sizeof(col)) { r= -iC; iC= 0; }
  t1= row[iC];
  digitalWrite(t0, HIGH);
  digitalWrite(t1, LOW);
  Serial.print('C'); Serial.print(t0,HEX); Serial.print("->"); Serial.println(t1,HEX); 
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
  
  if (sm & 0x40) { mm^= 0x1; }
  if (mm & 1)
  { // single step mode
    if (0== (sm & 0x80)) { sm= 0; } 
  }
#if 0
  rawStep(sm);
#else
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
#endif
  if (sm & 0x80) { Serial.println(sm,HEX); }
  else
  { // send some progress pips over host link
static uint8_t c0= 1, c1= 0;
    if (sm && (0 == --c0)) { Serial.print('.'); c0= 10; if (++c1 >= 20) { Serial.print('\n'); c1= 0; } }  
  }
  delay(50);
} // loop
