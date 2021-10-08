// Duino/Common/Morse/CMorse.hpp - International Morse Code pattern processing
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Feb - Mar 2021

#ifndef CMORSE_HPP
#define CMORSE_HPP

#include "morseAlphaNum.h"
#include "morsePuncSym.h"

// default 16bit! need to compile with --short-enum ?
//enum MorsePG { };
typedef int8_t MorsePG; // Pulse/Gap 3bits : b0 = on/off (llo)
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

// Generate sequence of 2*2b pulse codes from 2b codes within 16b sequence
int8_t pulseSeq2bIMC (uint8_t b[], uint16_t c, const int8_t n)
{
const uint8_t seq[]= { 0x55, 0x59, 0x95, 0x99 };
   int8_t i= 0;
   do
   {
      b[i]= seq[c & 0x3];
      c>>= 2;
   } while (++i < n);
   return(n << 1); // 2*2b pulse codes per bit (representing long/short on, followed by short off time, ie. 0bxx01)
} // pulseSeq2bIMC

struct B2Buff
{
   uint8_t b[4]; // Up to 8 pulses, max 7 needed ( $ symbol)
   int8_t n, i;

   bool set (const uint16_t codeBits, const int8_t nBits, const int8_t addLast=0x1)
   {
      i= n= pulseSeq2bIMC(b, codeBits, nBits);
      b[0]+= addLast;
      return(n > 0);
   }

   bool getNext (uint8_t& tc)
   {
      if (i > 0)
      {
         int8_t iB, i2b= --i;
         iB= i2b >> 2;
         i2b&= 0x3;
         tc= (b[iB] >> (2*i2b)) & 0x3;
         return(true);
      }
      return(false);
   } // getNext
}; // B2Buff

// Test/debug input ASCII text buffer with Arduino-Stream interface
class CMorseBuff : public Stream
{
protected:
   const char *sM;
   int8_t nM, iM;  // no long strings!

public:
   CMorseBuff () {;}

   void set (const char *msg)
   {
      sM= msg;
      if (msg) { nM= strlen(msg); } else { nM= 0; }
      nM+= (nM>0); // include trailing NUL to cleanly terminate message
      reset();
   }
   void reset (void) { iM= 0; }

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

   // Required for STM32 build, whereas AVR build ignores.. ??
   void flush (void) override { ; }
}; // CMorseBuff

// String Send State : generates output code sequence
class CMorseSSS
{
protected:
   CMorseBuff  sb;
   B2Buff b2b;
   int8_t addLastGap;
   uint8_t dbgFlag;

   bool setM5 (const uint8_t imc5, const int8_t addLast)
   {
      uint8_t c;
      int8_t n= unpackIMC5(&c, imc5);
      return b2b.set(c, n, addLast);
   }

   bool setM12 (const uint16_t imc12, const int8_t addLast)
   {
      uint16_t c;
      int8_t n= unpackIMC12(&c, imc12);
      if ((c & 0x3FF) == c) { return b2b.set(c, n, addLast); }
      return(false);
   }

   bool setASCII (const signed char a, const signed char c, const signed char nc)
   {
      int8_t addLast;
      switch(nc)
      {
         default : addLast= addLastGap; dbgFlag|= 0x1; break; // use current prosign state
         case ' ' : addLast= 2; dbgFlag|= 0x2; break;  // word gap
         case '>' : addLast= 2; dbgFlag|= 0x4; break;  // end of prosign, force word (?) gap
         case 0x0 : addLast= -1; dbgFlag|= 0x8; break; // end of message, remove trailing gap
     }
      switch (c)
      {
         case '0' : return setM5(gNumIMC5[a-c],addLast); // break;
         case 'a' :
         case 'A' : return setM5(gAlphaIMC5[a-c],addLast); // break;
         case '!' : return setM12(gSym1IMC12[a-c],addLast); // break;
         case ':' : return setM12(gSym2IMC12[a-c],addLast); // break;
         case '[' : if (a >= '_') { return setM12(gSym3IMC12[a-'_'],addLast); } break; // NB first 4 non-mappable skipped
      }
      return(false);
   } // setASCII

   bool nextASCII (Stream& s)
   {
      bool r=false;
      char a;
      do
      {
         if (0 == s.available()) { return(false); }
         a= s.read();
         switch(a)
         {
            case '<' : addLastGap= 0; dbgFlag|= 0x80; break; // prosign on
            case '>' : addLastGap= 1; dbgFlag|= 0x40; break; //      off
            default : r= setASCII( a, classifyASCII(a), classifyASCII(s.peek()) ); break;
         }
      } while (!r); // skip any non-translateable
      return(r);
   } // nextASCII

   void resetGap (void) { addLastGap=0x1; dbgFlag=0; }
public:
   uint8_t tc;

   CMorseSSS (void) { ; }

   void send (const char *msg) { sb.set(msg); resetGap(); }

   void resend (void)
   {
      sb.reset();
      resetGap();
   } // resend

   bool nextPulse (void)
   {
      if (b2b.getNext(tc) || (nextASCII(sb) && b2b.getNext(tc))) { tc|= pulseState()<<2; return(true); }
      //else
      tc= 0;
      return(false);
   } // nextPulse

   uint8_t pulseState (void) { return(b2b.i & 0x1); }

   // TODO : find better condition...
   bool ready (void) { return(sb.available() > 0); }
   bool complete (void) { return(sb.available() <= 0); }

}; // CMorseSSS

#define MORSE_FARNSWORTH_OFF  0x10
#define MORSE_FARNSWORTH_1Q12 0x12
#define MORSE_FARNSWORTH_1Q25 0x14
#define MORSE_FARNSWORTH_1Q50 0x18
#define MORSE_FARNSWORTH_1Q75 0x1C
#define MORSE_FARNSWORTH_2Q00 0x20
// Apply a simple timing model to coded output
class CMorseTime : public CMorseSSS
{
public:
   uint8_t msPulse; // 50~100 typically
   uint8_t q4FGS; // Gap stretch ratio as 4.4 fixed point fraction
   uint16_t t;

   CMorseTime (uint8_t nPerUnit, uint8_t timeUnit)
   {
      if (timeUnit > MORSE_FARNSWORTH_OFF) { q4FGS= timeUnit; } else { q4FGS= 0; }
      switch(timeUnit)
      {
         default : msPulse= 60000 / (nPerUnit * MORSE_CANONICAL_WORD); break; // wpm
         case 0 : msPulse= 500 / nPerUnit; break; // dps
      }
   }

   bool nextPulse (void)
   {
      bool r= CMorseSSS::nextPulse();
      if (0 != q4FGS)
      {  // Farnsworth gap stretching
         switch(tc & 0x7)
         {
            case 0x2 : t= (3 * q4FGS * msPulse) >> 4; return(r);
            case 0x3 : t= (7 * q4FGS * msPulse) >> 4; return(r);
         }
      }
      //else
      switch(tc & 0x3)
      {
         case 0x1 : t= msPulse; break;
         case 0x2 : t= 3*msPulse; break;
         case 0x3 : t= 7*msPulse; break;
         default: // case 0x0 : 
            t= 0; break;
      }
      return(r);
   } // nextPulse

   // (<using> clauses necessary for private inheritance)
   //using CMorseSSS::send;
   //using CMorseSSS::resend;
   //using CMorseSSS::ready;
   //using CMorseSSS::complete;

}; // CMorseTime

#include "../SerMux.hpp" // host receiver in progress

class CMorseDebug : public CMorseTime
{
public :
   CMorseDebug (uint8_t np=25, uint8_t tu=MORSE_FARNSWORTH_1Q50) : CMorseTime(np,tu) { ; }

   bool nextPulse (Stream& log)
   {
      bool r= CMorseTime::nextPulse();
      static const char dbg[8]={ '\n', 0, '|', ' ' , '0', '.', '-', '3' };
#ifdef SERMUX_HPP
      CSerMux m; m.send(log, 0x30, dbg+(tc&0x7),1);
#else
      log.write( dbg[tc&0x7] );
#endif
      log.flush();
      //if (dbgFlag & 0x0C) { log.print("f:"); log.println(dbgFlag,HEX); }
      return(r);
   } // nextPulse

   void info (Stream& log)
   {
     log.print("msPulse="); log.println(msPulse);
     log.print("Farnsworth="); log.println(q4FGS,HEX);
   } // info

}; // CMorseDebug

#endif // CMORSE_HPP
