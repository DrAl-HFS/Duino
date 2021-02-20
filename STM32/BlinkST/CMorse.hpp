// Duino/STM32/BlinkST/CMorse.hpp - International Morse Code pattern processing
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Feb 2021

#ifndef CMORSE_HPP
#define CMORSE_HPP

#include "morsePattern.h"

/* DEPRECATE: Pattern Send State - 
class CMorsePSS 
{
protected:
   uint8_t c;
   int8_t n, i;

   void set (const uint8_t codeBits, const int8_t nBits) { c= codeBits; n= nBits; i= n; }
  
public:
   CMorsePSS (void) { ; }

   bool setSafe (const uint8_t codeBits, const int8_t nBits)
   { 
      if ((nBits < 0) || (nBits > 8)) { return(false); } //else
      set(codeBits, nBits);
      return(true);
   } // set 
  
   bool setM5 (const uint8_t m5=0) { set(m5 & IMC5_C_MASK, m5 >> IMC5_N_SHIFT); return(true); }
  
   int8_t next (void)
   {
      int8_t v= 0x2; // end of pattern
      if (i >= 0) { v= (c >> --i) & 0x1; }
      return(v);
   }
}; // CMorsePSS
*/
// classify as: decimal digit, upper/lower case alpha, symbol, whitespace&control, nul, invalid
// returns representative character: 0, a, A, !, 1, 0, -1
signed char classifyASCII (const signed char a)
{
   if (a >= '0')
   {
      if (a <= '9') { return('0'); } 
      if (a >= 'A')
      {
         if (a <= 'Z') { return('A'); }
         if ((a >= 'a') && (a <= 'z')) { return('a'); }
      }
   }
   if ((a > ' ') && (a < 0x7F)) { return('!'); }
   return((a > 0) - (a < 0));
} // classifyASCII

// generate sequence of 2b pulse codes from <=8 bit code
int8_t pulseSeq2bIMC (uint8_t b[], uint8_t c, const int8_t n)
{
const uint8_t seq[]= { 0x00, 0x04, 0x40, 0x44 };
   int8_t i= 0;
   do
   {
      b[i]= seq[c & 0x3];
      c>>= 2;
   } while (++i < n);
   return(n << 1); // 2*2b pulse codes per bit (representing long/short on, followed by short off time)
} // pulseSeq2bIMC

struct B2Buff
{
   uint8_t b[4];
   int8_t n, i, l;

   void set (const uint8_t codeBits, const int8_t nBits, const int8_t next=0x2)
   {
      i= n= pulseSeq2bIMC(b, codeBits, nBits);
      if (next > 0) { b[0]|= next & 0x3; } // set next symbol / word gap
      else { l= 1; } // end without trailing gap
   }
  
   bool getNext (uint8_t& v)
   {
      if (i > l)
      {
         int8_t iB, i2b= --i;
         iB= i2b >> 2;
         i2b&= 0x3;
         v= (b[iB] >> (2*i2b)) & 0x3;
         return(true);
      }
      return(false);
   } // getNext
}; // B2Buff

// String Send State
class CMorseSSS
{
protected:
   const char *s;
   int8_t iS;  // no long strings!
   B2Buff b2b;
   
   bool setM5 (const uint8_t imc5)  // CMorsePSS::setM5(imc5);
   {
      uint8_t c;
      int8_t n= unpackIMC5(&c, imc5);
      //printf("c,n=0x%X,%d ", c, n);
      b2b.set(c, n);
      //printf("b2b=%d,0x%02X,%02X,%02X,%02X\n", b2b.n, b2b.b[3], b2b.b[2], b2b.b[1], b2b.b[0]);
      return(true);
   }
   bool setASCII (const signed char a)
   {
      const signed char c= classifyASCII(a);
      printf("a=%c : ", a);
      switch (c)
      {
         case '0' :  return setM5(gNumIMC5[a-c]); // break;
         case 'a' :  //return setM5(gAlphaIMC5[a-c]); // break;
         case 'A' :  return setM5(gAlphaIMC5[a-c]); // break;
         //case '!' : break; // no map yet
      }
      return(false);
   } // setASCII
  
   bool nextASCII (void)
   {
      char a;
      if (NULL == s) { return(false); }
      do
      {
         a= s[++iS];
         if (0 == a) { return(false); }
      } while (!setASCII(a)); // skip any non-translateable
      return(true);
   } // nextASCII
  
public:
   uint8_t v, t;
   
   CMorseSSS (void) { ; }
  
   void set (const char *str)
   { 
      s= str;
      reset();
   } // set
  
   void reset (void)
   {
      iS= 0;
      if (NULL == s) { setASCII(0); }
      else { setASCII(s[iS]); }
   } // reset
/*
   int8_t next (void)
   {
      int8_t r= CMorsePSS::next();
      if (r >= 0x2)
      { 
         if (!nextASCII()) { r|= 0x4; }
      }
      return(r);
   } // next
*/   
   bool nextPulse ()
   {
      if (b2b.getNext(t) || (nextASCII() && b2b.getNext(t)))
      {
         v= (b2b.i & 0x1); // odd numbered on, even off
#if 1
{
  static const char dbg[2][4]={ { 0x0, 'e', '|',' '}, {'.','-','e','e'} };
  char ch=dbg[v][t];
  if (ch)
  {
     printf("%c",ch);
     //DEBUG.write(ch); DEBUG.flush();
  }
}
#endif
         t= gTimeRelIMC[t]; // * timeScale;
         return(true);
      }
      else { v= t= 0; }
      return(false);
   } // nextPulse
   
   void print (void) { printf("[%d] %d*%d\n", b2b.i, v, t); }
   
}; // CMorseSSS

#endif // CMORSE_HPP
