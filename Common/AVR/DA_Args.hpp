// Duino/Common/DA_SerCmd.h - Arduino (AVR) specific (AD9833 control related) text IO processing
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Nov-Dec 2020

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


#define BCD8_MAX 3
#define BCD4_MAX (2*BCD8_MAX)

class CNumBCDX
{
protected:
   uint8_t bcd[BCD8_MAX], x;
   //int8_t   e;    // TODO : pack as 2 * 4bit
   //uint8_t  nU;  // extended info, # additional digits 0..4, flags? valid? sign?

   bool setE (int8_t e)
   {
      uint8_t m= e;
      if (e < 0) { e= -e; m= 0x08 | e; }
      if (e > 7) { return(false); }
      x= (x & 0xF0) | m;
      return(true);
   } // setE

   bool setD (uint8_t d)
   {
      if (d > BCD4_MAX) { return(false); }
      uint8_t m= swapHiLo4U8(d & 0x7);
      x= (x & 0x0F) | m;
      return(true);
   } // setD
   
   bool setClkD (uint8_t d) { if (setD(d)) { x= (0x88 | x) & 0xF8; } } // set flags, clear exp

   int8_t getE (void)
   {
      int8_t e= 0x07 & x;
      if (0 == (x & 0x08)) { return(e); }
      else { return(-e); }
   } // getE

   uint8_t getD (void)
   {
      int8_t d= 0x07 & swapHiLo4U8(x);
      if (d <= 6) { return(d); }
      else { return(0); }
   } // getD

public:
   CNumBCDX () {;}
   
   void dumpHex (Stream & s)
   {
      s.print("m0x"); s.print(bcd[0],HEX); s.print(bcd[1],HEX); s.println(bcd[2],HEX);
      s.print("E"); s.print(getE()); s.print("D"); s.println(getD());
   }

   bool isNum (void) const { return((0x70 & x) > 0); }
   bool isClk (void) const { return(0x88 == (0x8F & x)); }

   // Store digits packed in big endian order
   uint8_t readStreamBCD (Stream& s, uint8_t i, const uint8_t max)
   {
      while ( isDigit(s.peek()) && (i < max))
      {
         uint8_t d= s.read()-'0';
         if (i & 1) { bcd[i>>1]|= d; } else { bcd[i>>1]= swapHiLo4U8(d); }
         ++i;
      }
      return(i);
   } // readStreamBCD

   int8_t readStreamClock (Stream& s, const uint8_t a)
   {
      uint8_t nD= 0, nO= 0;
      char ch;
      
      do
      {
         uint8_t d= readStreamBCD(s, nD, min(a,BCD4_MAX));
         ch= 0;
         // s.print(d); dumpHex(s);
         if ((d > nD) && (d < BCD4_MAX))
         {
            if (':' == s.peek()) { ch= s.read(); ++nO; }
         }
         nD= d;
      } while ((ch>0) && (nD < BCD4_MAX));
      setClkD(nD);
      return(nD+nO);
   } // readStreamClock
   
   int8_t readStream (Stream& s, const uint8_t a)
   {
      uint8_t point= 0xFF, nO=0, nD= readStreamBCD(s, 0, min(a,BCD4_MAX));
      int8_t i, e= 0; // TODO : merge i->e

      if (nD > 0)
      {
         i= idxMatch(s.peek(),"Mk.muhdEe");
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
                  nD= readStreamBCD(s, nD, min(a,BCD4_MAX-nD));
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
         setE(e);
         setD(nD);
      }
      return(nD+nO);
   } // readStream
  
   uint8_t extractUME (uint32_t& mx, int8_t& ex, const uint8_t maxD=4)
   {
       uint8_t nD= getD(), d= min( maxD, nD );  // limit conversion (prevent overflow)..
       ex= getE() + d - nD; // adjust exponent
       mx= u32FromBCD(bcd,nD); // convert
       return(nD);
   } // extractUME
   
   uint32_t extractScale (const uint16_t mScale=1, const int8_t eScale=3)
   {
      uint32_t mx;
      int8_t ex;
      extractUME(mx,ex); // > 0) ???
      //Serial.print("extract() - mx,ex,es"); Serial.print(mx); Serial.print(','); Serial.print(ex); Serial.print(','); Serial.println(eScale);
      ex+= eScale;
      if (1 != mScale) { mx*= mScale; }
      if (0 == ex) { return(mx); }
      else if (ex > 0) { return(mx * uexp10(ex)); }
      //else if (ex < 0) { 
      return(mx / uexp10(-ex));
   } // extractScale
   
   int8_t extractHMS (uint8_t hms[3], int8_t n)
   {
      int8_t i=0;
      if (!isClk()) { Serial.println("!isClk()"); } // ????
      
      {  // dumpHex(Serial);
         uint8_t b= (1 + getD()) / 2; // round up, just in case
         n= min(n, b);
         while (i < n) { hms[i]= fromBCD4(bcd[i], 2); ++i; }
      }
      return(i);
   } // extractHMS
   
}; // CNumBCDX

// TODO : cleanup command flags
#define SCI_VAL_MAX 4
#define SEP_CH_MAX 8
class CmdSeg
{
public:
  CNumBCDX v[SCI_VAL_MAX];
  char sep[SEP_CH_MAX];
  uint8_t nFV, cmdF[2], cmdR[2];
  int8_t  iRes;

  uint8_t getNV (void) const { return(nFV & 0xF); }
  void clean () { nFV=0; cmdF[0]= 0; cmdF[1]= 0; cmdR[0]= 0; cmdR[1]= 0; iRes= 0; }
  bool operator () (void) const { return(0 != nFV); }
};

#endif // DA_ARGS_HPP
