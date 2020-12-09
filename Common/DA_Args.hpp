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


#define BCD8_MAX 3
#define BCD4_MAX (2*BCD8_MAX)

class USciExp
{
protected:
   uint8_t bcd[BCD8_MAX];
   int8_t   e;    // TODO : pack as 2 * 4bit
   uint8_t  nU;  // extended info, # additional digits 0..4, flags? valid? sign?

   
public:
   USciExp () {;}
   
   void dumpHex (Stream & s)
   {
      s.print("m0x"); s.print(bcd[0],HEX); s.print(bcd[1],HEX); s.println(bcd[2],HEX);
      s.print("e"); s.print(e); s.print("u"); s.println(nU);
   }

   bool isNum (void) const { return(nU>0); }
   bool isClk (void) const { return(0x80 == e); }

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
         uint8_t d= readStreamBCD(s, nD, BCD4_MAX);
         ch= 0;
         // s.print(d); dumpHex(s);
         if ((d > nD) && (d < BCD4_MAX))
         {
            if (':' == s.peek()) { ch= s.read(); ++nO; }
         }
         nD= d;
      } while ((ch>0) && (nD < BCD4_MAX));
      nU= nD;
      e= 0x80; // hacky...
      return(nD+nO);
   } // readStreamClock
   
   int8_t readStream (Stream& s, const uint8_t a)
   {
      uint8_t point= 0xFF, nO=0, nD= readStreamBCD(s, 0, min(a,BCD4_MAX));

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
         nU= nD;
      }
      return(nD+nO);
   } // readStream
  
   uint8_t extractUME (uint32_t& mx, int8_t& ex, const uint8_t maxD=4)
   {
       uint8_t nD= min(maxD,nU);  // limit conversion (prevent overflow)..
       ex= e + nD - nU; // adjust exponent
       mx= u32FromBCD(bcd,nD); // convert
       return(nD);
   } // extractUME
   
   uint32_t extract (const uint16_t mScale=1, const int8_t eScale=3)
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
   } // extract
   
   int8_t extractHMS (uint8_t hms[3], int8_t n)
   {
      int8_t i=0;
      if (!isClk()) { Serial.println("!isClk()"); } // ????
      
      {  // dumpHex(Serial);
         n= min(n, nU/2);
         while (i < n) { hms[i]= fromBCD4(bcd[i], 2); ++i; }
      }
      return(i);
   } // extractHMS
   
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
