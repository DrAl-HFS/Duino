// Duino/UBit/blink/MapLED.hpp - Micro:Bit LED map
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Jan 2021

#ifndef MAP_LED_HPP
#define MAP_LED_HPP

#include "Common/N5/N5_Util.hpp"

// This should be encapsulated (in class) but seems to be an old gcc version
// lacking proper C11 support...

#ifdef TARGET_UBITV1
// Arduino pin numbers
static const uint8_t row[3]={26,27,28}; // Active high (source)
static const uint8_t col[9]={3,4,10,23,24,25,9,7,6}; // Active low (sink)
// 5 * 5 matrix mapping using bytes as nybble pairs (tuples) in row:col index order
static const uint8_t m55[5][5]=
{
  { 0x00, 0x13, 0x01, 0x14, 0x02, },
  { 0x23, 0x24, 0x25, 0x26, 0x27, },
  { 0x11, 0x08, 0x12, 0x28, 0x10, },
  { 0x07, 0x06, 0x05, 0x04, 0x03, },
  { 0x22, 0x16, 0x20, 0x15, 0x21 }
};
#endif // TARGET_UBITV1

#ifdef TARGET_UBITV2 // Just an (incorrect) copy of V1 for now...
// https://microbit-micropython.readthedocs.io/en/v2-docs/pin.html
static const uint8_t row[5]={21,22,23,24,25}; // Active high (source)
static const uint8_t col[5]={4,7,3,6,10}; // Active low (sink)
/*
static const uint8_t m55[5][5]=
{
  { 0x00, 0x01, 0x02, 0x03, 0x04, },
  { 0x10, 0x11, 0x12, 0x13, 0x14, },
  { 0x20, 0x21, 0x22, 0x23, 0x24, },
  { 0x30, 0x31, 0x32, 0x33, 0x34, },
  { 0x40, 0x41, 0x42, 0x43, 0x44 }
};
* */
#endif // TARGET_UBITV1

// UBit V2 defs ???

class CMapLED // row-col multiplex 3*9 physical <-> 5*5 logical (+2???)
{
public:
   CMapLED (void) {;}
   
   void init (int state=-1)
   {
      for (int i=0; i<sizeof(row); i++) { pinMode(row[i], OUTPUT); }
      for (int i=0; i<sizeof(col); i++) { pinMode(col[i], OUTPUT); }
      if (state >= 0)
      {
         setRow(1);
         setCol(state^1);
      }
   }

   void setRow (int v)
   {
     v= (0 != v);
     for (int i=0; i<sizeof(row); i++) { digitalWrite(row[i], v); }
   } // setRow

   void setCol (int v)
   {
     v= (0 != v);
     for (int i=0; i<sizeof(col); i++) { digitalWrite(col[i], v); }
   } // setCol
   
   uint8_t getRCI (uint8_t iR, uint8_t iC)
   { 
#ifdef TARGET_UBITV2 
      return((iR << 4)|iC);
#else
      return m55[iR][iC];
#endif
  }
   void rowSwitch (uint8_t r0, uint8_t r1)
   {
      if (r0 != r1)
      {
         digitalWrite(row[r0], LOW);
         digitalWrite(row[r1], HIGH);
      }
   }
   void colSwitch (uint8_t c0, uint8_t c1)
   {
      if (c0 != c1)
      {
         digitalWrite(col[c0], HIGH);
         digitalWrite(col[c1], LOW);
      }
   }
}; // CMapLED

#endif // MAP_LED_HPP
