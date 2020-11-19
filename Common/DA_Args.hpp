// Duino/Common/DA_SerCmd.h - Arduino (AVR) specific serial command processing
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Nov 2020

#ifndef DA_ARGS_HPP
#define DA_ARGS_HPP

#include "DA_Util.h"


/***/

uint32_t uexp10 (const int8_t e)
{ 
   if (e >= 1)
   {
      uint32_t s=10;
      for (int8_t i=1; i<e; i++) { s*= 10; }
      return(s);
   }
   return(e > 0);
} // uexp10

// indexOf(String) ? clunky ?
int8_t idxMatch (const char ch, const char s[])
{
   int8_t i=0;
   while (s[i] && (ch != s[i])) { ++i; }
   if (ch == s[i]) { return(i); }
   //else
   return(-1);
} // idxMatch

bool isDecIntCh (const char ch) { return( isDigit(ch) || ('-' == ch) ); }
// Avoid float
//uint32_t f2FSR (float f) { return(f * (((uint32_t)1<<28) / 25E6)); }
#define F_TO_FSR(f) (f * (((uint32_t)1<<28) / 25E6))

class USciExp
{
public:
  union { uint8_t bcd[4]; uint16_t u; }; // NB: u overlaps first 2 bytes of bcd (4 digits)
  int8_t   e;
  uint8_t  nU;  // extended info, # additional digits 0..4, flags? valid? sign?
  
  USciExp () {;}
  
  // 2^28 / 25e6 = 10.737(418..) = 10737E-3
  uint32_t toFSR () const  // this doesn't belong here...
  {
      if (u > 0)
      { 
#if 0
        float f= u;
        if (e > 0) { f*= uexp10(e); }
        Serial.print(f);Serial.println("Hz");
        if (f < 12.5E6) { return f2FSR(f); }
#else
        uint32_t n= (uint32_t)u * 10737; // 3 fractional digits in scale
        Serial.print(u);Serial.print('e');Serial.print(e);Serial.println("Hz");
        if (3 == e) { return n; }
        else if (e > 3) { return n * uexp10(e-3); }
        else if (e < 3) { return n / uexp10(3-e); }
    //Serial.print(u);Serial.print('e');Serial.print(e);Serial.print("->");Serial.print(n);Serial.print("?");Serial.println(s);
#endif
      }
      return(0); // not representable in target (hardware) format
   } // toFSR

  uint32_t fromBCD (uint8_t bcd[], const uint8_t n) // not really a class member...
  {
    uint32_t r= fromBCD4(bcd[0], n);
    if (n > 2)
    {
      uint8_t i= 1;
      while (i < (n/2))
      {
         r= r * 100 + fromBCD4(bcd[i], n-2*i);
         ++i;
      }
      if (n & 1) { r= r * 10 + fromBCD4(bcd[i], 1); }
    }
    return(r);
  } // fromBCD

  // Store digits packed in big endian order
  uint8_t readStreamBCD (Stream& s, uint8_t bcd[], uint8_t i, const uint8_t max)
  {
    while ( isDigit(s.peek()) && (i < max))
    {
      uint8_t d= s.read()-'0';
      if (i & 1) { bcd[i>>1]|= d; } else { bcd[i>>1]= swapHiLo4U8(d); }
      ++i;
    }
    return(i);
  } // readStreamBCD
  
  int8_t readStream (Stream& s, const uint8_t a)
  {
    uint8_t point= 0xFF, nO=0, nD= readStreamBCD(s, bcd, 0, min(a,8));
    
    if (nD > 0)
    {
      e= 0;
      int8_t i= idxMatch(s.peek(),"Mk.muhdEe");
      if (i >= 0)
      {
        char ch= toupper(s.read()); nO++; // discard
        if (i < 7)
        { 
          switch(i)
          {
            case 6 : e= -1; break; // deci
            case 5 : e= 2; break; // hecto
            default : e= 6 - 3 * i; break; // Mega kilo 0 milli micro
          }
          point= nD; 
          if ((a > (nD+1)) && isDigit(s.peek()))
          { // fractional part
            nD= readStreamBCD(s, bcd, nD, min(a,8-nD));
          }
          if ('.'==ch) // point ws given, might be E# notation
          {
            ch= toupper(s.peek());
            if ('E'==ch) { s.read(); nO++; }
          }
        }
        if (('E'==ch) && isDecIntCh(s.peek())) { e+= s.parseInt(SKIP_NONE); } // integer exponent (unknown digit count...)
        if (nD > point) { e-= nD - point; } // fraction digits
      }
      nU= min(4,nD);  // limit conversion (prevent subsequent overflow)..
      e+= nD - nU; // adjust exponent as necessary
      // TODO : revise format, defer conversion - maximise precision
      u= fromBCD(bcd,nU); // convert, overwriting bcd[0..1]
      nU= nD-nU; // number of bcd digits remaining from bcd[2] onward
    }
    return(nD+nO);
  } // readStream
}; // USciExp

#define SCI_VAL_MAX 4
#define SEP_CH_MAX 8
class CmdSeg
{
public:
  USciExp v[SCI_VAL_MAX];
  char sep[SEP_CH_MAX];
  uint8_t nFV, cmdF[2], cmdR[2];
  
  uint8_t getNV (void) const { return(nFV & 0xF); }
  void clean () { nFV=0; cmdF[0]= 0; cmdF[1]= 0; cmdR[0]= 0; cmdR[1]= 0; }
  operator () (void) const { return(0 != nFV); }
};

#endif // DA_ARGS_HPP
