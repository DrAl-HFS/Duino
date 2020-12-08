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
   return(e >= 0);
} // uexp10

// indexOf(String) ? clunky ?
// return index of first match
int8_t idxMatch (const char ch, const char s[])
{
   int8_t i=0;
   while (s[i] && (ch != s[i])) { ++i; }
   if (ch == s[i]) { return(i); }
   //else
   return(-1);
} // idxMatch

bool isDecIntCh (const char ch) { return( isDigit(ch) || ('-' == ch) ); }

// displace ???
uint32_t u32FromBCD (uint8_t bcd[], const uint8_t n) // not really a class member...
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
} // u32FromBCD


#define BCD8_MAX 4
#define BCD4_MAX (2*BCD8_MAX)

class USciExp
{
public:
   uint8_t bcd[BCD8_MAX];
   int8_t   e;
   uint8_t  nU;  // extended info, # additional digits 0..4, flags? valid? sign?

   USciExp () {;}

   // 2^28 / 25e6 = 10.737(418..) = 10737E-3
   uint32_t toFSR () const  // FIX: using information belonging elsewhere
   {
      if (nU > 0)
      {
#if 0
        float f= u;
        if (e > 0) { f*= uexp10(e); }
        Serial.print(f);Serial.println("Hz");
        if (f < 12.5E6) { return f2FSR(f); }
#endif
#if 0
        uint32_t mx;
        int8_t ex;
        uint32_t n;
        extractUME(mx,ex);
        n= (uint32_t)mx * 10737; // 3 fractional digits in scale
        Serial.print(mx);Serial.print('e');Serial.print(ex);Serial.println("Hz");
        if (3 == ex) { return n; }
        else if (ex > 3) { return n * uexp10(ex-3); }
        else if (ex < 3) { return n / uexp10(3-ex); }
#else
        return extract(10737,-3);
#endif
      }
      return(0); // not representable in target (hardware) format
   } // toFSR

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
      uint8_t point= 0xFF, nO=0, nD= readStreamBCD(s, bcd, 0, min(a,BCD4_MAX));

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
                  nD= readStreamBCD(s, bcd, nD, min(a,BCD4_MAX-nD));
               }
               if ('.' == ch) // point char was found, check for exp e#/E# notation
               {
                  ch= toupper(s.peek());
                  if ('E'==ch) { s.read(); nO++; }
               }
            }
            if (('E'==ch) && isDecIntCh(s.peek())) { e+= s.parseInt(SKIP_NONE); } // integer exponent (unknown digit count...)
            if (nD > point) { e-= nD - point; } // fraction digits
         }
         nU= nD;
      }
      return(nD+nO);
   } // readStream
  
   uint8_t extractUME (uint32_t& mx, int8_t& ex, const uint8_t maxD=4)
   {
       uint8_t nD= min(maxD,nU);  // limit conversion (prevent overflow)..
       ex= e + nD - nU; // adjust exponent
       mx= u32FromBCD(bcd,nD); // convert, overwriting bcd[0..1]
       return(nD);
   } // extractUME
   
   uint32_t extract (const uint16_t mScale=1, const int8_t eScale=3)
   {
      uint32_t mx;
      int8_t ex;
      extractUME(mx,ex); // > 0) ???
      Serial.print("extract() - mx,ex,es"); Serial.print(mx); Serial.print(','); Serial.print(ex); Serial.print(','); Serial.println(eScale);
      ex+= eScale;
      if (1 != mScale) { mx*= mScale; }
      if (0 == ex) { return(mx); }
      else if (ex > 0) { return(mx * uexp10(ex)); }
      //else if (ex < 0) { 
      return(mx / uexp10(-ex));
   } // extract
   
}; // USciExp

// TODO : cleanup command flags
#define SCI_VAL_MAX 4
#define SEP_CH_MAX 8
class CmdSeg
{
public:
  USciExp v[SCI_VAL_MAX];
  char sep[SEP_CH_MAX];
  uint8_t nFV, cmdF[2], cmdR[2];
  int8_t  iRes;

  uint8_t getNV (void) const { return(nFV & 0xF); }
  void clean () { nFV=0; cmdF[0]= 0; cmdF[1]= 0; cmdR[0]= 0; cmdR[1]= 0; iRes= 0; }
  bool operator () (void) const { return(0 != nFV); }
};

#endif // DA_ARGS_HPP
