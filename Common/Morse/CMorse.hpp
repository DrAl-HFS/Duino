// Duino/Common/Morse/CMorse.hpp - International Morse Code pattern processing
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Feb 2021

#ifndef CMORSE_HPP
#define CMORSE_HPP

#include "morseAlphaNum.h"
#include "morsePuncSym.h"

// default 16bit! need to compile with --short-enum ?
//enum MorsePG { };
typedef int8_t MorsePG; // Pulse/Gap 3bits : b0 = on/off
#define MORSE_PG_NONE  ((MorsePG)0x0)
#define MORSE_PG_ON    ((MorsePG)0x1)
#define MORSE_PG_SHORT ((MorsePG)0x2)
#define MORSE_PG_LONG  ((MorsePG)0x4)
#define MORSE_PG_INTER ((MorsePG)0x6)
#define MORSE_PULSE_SHORT (MORSE_PG_ON|MORSE_PG_SHORT)
#define MORSE_PULSE_LONG  (MORSE_PG_ON|MORSE_PG_LONG)
#define MORSE_PG_MASK  ((MorsePG)0x7)


// TODO : Revised pattern scan-out with implicit generation of spacing
class CMorseSeq
{
protected:
   uint16_t c;
   int8_t i;
   uint8_t ng; // combined n & gap mask
   
public:
   //uint8_t ss; // stored copy of send state ?

   CMorseSeq (void) { ; }

   void set (const uint16_t codeBits, const int8_t nBits, const MorsePG last=MORSE_PG_LONG)
   {
      c= codeBits;
      i= nBits << 1;
      ng= last | (i<<3); // worth storing n ??
   }
   int8_t nextState (void)
   {  // bits 1,0 time code, bit 2= pulse on / gap off
      int8_t ss=0;
      if (i > 0)
      {  // determine on/off state
         if (--i & 0x1)
         {  // pulse
            if (c & (1 << (i>>1))) { ss= MORSE_PULSE_LONG; }
            else { ss= MORSE_PULSE_SHORT; }
         }
         else
         {  // gap
            if (0 == i) { ss= MORSE_PG_MASK & ng; } //whatever was set (off, short, intersymbol, inter-word)
            else { ss= MORSE_PG_SHORT; } // short
         } 
      }
      return(ss);
   }
}; // CMorseSeq


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
   if ((' ' == a) || ('\t' == a)) return(' ');
   if (a > ' ')
   {  // symbol groups (contiguous ASCII codes)
      if (a < '/') { return('!'); }
      if ((a >= ':') && (a <= '@')) { return(':'); }
      if ((a >= '[') && (a <= '`')) { return('['); }
      if ((a >= '{') && (a <= '~')) { return('{'); }
   }
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

   void set (const uint8_t codeBits, const int8_t nBits, const int8_t last=0x2)
   {
      i= n= pulseSeq2bIMC(b, codeBits, nBits);
      if (last > 0) { b[0]|= last & 0x3; l= 0; if (i<= 0) { i= 1; } } // set next symbol / word gap
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

// Test/debug buffer with Arduino-Stream interface
class CMorseBuff : public Stream
{
protected:
   const char *sM;
   int8_t nM, iM;  // no long strings!
   
public:
   CMorseBuff () {;}
   
   void set (const char *msg) { sM= msg; nM= strlen(msg); reset(); }
   void reset (void) { iM= 0; }
   
   // Hacks
   int ipeek (uint8_t i)
   {
      if (available() > i) { return(sM[i]); } //else
      return(-1);
   } // ipeek
   //int8_t iget (void) { return(iM); }
   
   // Stream interface methods
   int available (void) override
   {
      if (sM) { return(nM-iM); } // else
      return(0);
   } // available
   int peek (void) override
   {
      if (available()) { return(sM[iM]); } //else
      return(-1);
   } // peek
   int read (void) override
   {
      if (available()) { return(sM[iM++]); } //else
      return(-1);
   } // read
   size_t write (uint8_t b) override { return(0); }
}; // CMorseBuff

// String Send State
class CMorseSSS : CMorseBuff
{
protected:
   B2Buff b2b;
   
   bool setM5 (const uint8_t imc5, const int8_t last)
   {
      uint8_t c;
      int8_t n= unpackIMC5(&c, imc5);
      b2b.set(c, n, last);
      return(true);
   }
   bool setM12 (const uint16_t imc12, const int8_t last)
   {
      uint16_t c;
      int8_t n= unpackIMC12(&c, imc12);
      if ((c & 0x3FF) == c) { b2b.set(c, n, last); return(true); }
      return(false);
   }
   bool setASCII (const signed char a, const signed char c, const signed char nc)
   {
      int8_t last= 0x2 + (' ' == nc); // - (0x00 == nc);
      switch (c)
      {
         case '0' : return setM5(gNumIMC5[a-c],last); // break;
         case 'a' :
         case 'A' : return setM5(gAlphaIMC5[a-c],last); // break;
         case '!' : return setM12(gSym1IMC12[a-c],last); // break;
         case ':' : return setM12(gSym2IMC12[a-c],last); // break;
         case '[' : if (a >= '_') { return setM12(gSym3IMC12[a-'_'],last); } break; // NB first 4 non-mappable skipped
         // ???case 0x80: if (a <= 0x8C) { return setM12(gProsIMC12[a-0x80],last); } break;
      }
      return(false);
   } // setASCII
  
   bool nextASCII (Stream& s)
   {
      char a;
      if (0 == s.available()) { return(false); }
      do
      {
         a= s.read(); //if (0 == a) { return(false); }
      } while ( !setASCII( a, classifyASCII(a), classifyASCII(s.peek()) ) ); // skip any non-translateable
      return(true);
   } // nextASCII
  
public:
   uint8_t v, t;
   
   CMorseSSS (void) { ; }
  
   void send (const char *msg) { CMorseBuff::set(msg); }
   
   void resend (void)
   {
      CMorseBuff::reset();
      if (this->available())
      {
         char a= CMorseBuff::peek();
         char c= classifyASCII(a);
         setASCII(a,c,c); // hacky!
      } else { setASCII(0,0,0); }
   } // resend

   bool nextPulse (void)
   {
      if (b2b.getNext(t) || (nextASCII(*this) && b2b.getNext(t)))
      {
         v= (b2b.i & 0x1); // odd numbered on, even off
#if 1
{  // '\n'
  static const char dbg[2][4]={ { 0x0, 'e', '|',' '}, {'.','-','e','e'} };
  char ch=dbg[v][t];
  if (ch)
  {
     //printf("%c",ch);
     DEBUG.write(ch); DEBUG.flush();
  }
}
#endif
         t= gTimeRelIMC[t]; // * timeScale;
         return(true);
      }
      else { v= t= 0; }
      return(false);
   } // nextPulse

   // TODO : find better condition...
   bool ready (void) const { return(this->available() > 0); }
   bool complete (void) const { return(this->available() <= 0); }
   
   void print (void) { printf("[%d] %d*%d\n", b2b.i, v, t); }
   
}; // CMorseSSS

#endif // CMORSE_HPP
