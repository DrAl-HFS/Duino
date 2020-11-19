// Duino/Common/DA_SerCmd.h - Arduino (AVR) specific serial command processing
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Nov 2020

#ifndef DA_STRM_CMD_HPP
#define DA_STRM_CMD_HPP

#include "DA_Args.hpp"


/***/

class StreamFilter
{
private:
   uint8_t last; // do not share instance...
public:
   StreamFilter () { ; }
  
protected:
   // defer processing until significant input or
   // no change for #=5 cycles (1ms => 12chars @ 1115.2kBd)
   uint8_t avail (Stream& s)
   {
      uint8_t a= s.available();
      if (a < 16)
      {
         if (a < 1) { last= a; return(0); } 
         if (a > (last & 0xF)) { last= a; }
         else { last+= 0x10; } // count cycles
         if (last < 0x51) { return(0); }
      }
      return(a);
   } // avail
  
   void resid (uint8_t r) { last= min(r,0xF); }
};

void help (Stream& s)
{
#if 0
// PROGMEM - ineffective on multi-dimensional array?
static 
const char* st[]=
{
   "*\t",
   "S T C D - waveform: Sin,Tri,Clock,Double",
   "F - Function (cyclic)",
   "H O R - set: Hold,On/Off,Reset",
   "? - help (this text)"
};
static const int8_t nST= sizeof(st)/sizeof(st[0]);
   s.println(st[0]);
   for (int8_t i=1; i<nST; i++) { s.print(st[0]); s.println(st[i]); }
   s.println(st[0]);
#else
   s.println('*');
   s.print(F("*\t")); s.println(F("S T C D - waveform: Sin,Tri,Clock,Double"));
   s.print(F("*\t")); s.println(F("F - Function (cyclic)"));
   s.print(F("*\t")); s.println(F("H O R - set: Hold,On/Off,Reset"));
   s.print(F("*\t")); s.println(F("? - help (this text)"));
   s.println('*');
#endif
} // help

class StreamCmd : StreamFilter
{
protected:
   uint8_t n, a; // crap hack
  
   uint8_t scanV (USciExp v[], char sep[], const uint8_t max, Stream& s)
   {
      uint8_t n0, i=0, j=0;
      char ch;
      do // Read a sequence of separated numbers
      {  
         n0= n;
         n+= v[i].readStream(s,a);
         i+= (n > n0);
         do
         {
            ch= idxMatch(s.peek(), ",;:*&@\/#$%"); // space? tab ?
            if (ch >= 0)
            {
               ch= s.read(); n++; 
               if (j < SEP_CH_MAX) { sep[j++]= ch; }
            }
         } while (ch > 0);
      } while ((n < a) && (n > n0) && (i < max));
      if (j < SEP_CH_MAX) { sep[j]= 0; }
      return(i);
   } // scanV

   uint8_t scanC (uint8_t f[2], Stream& s)
   {
      uint8_t n0, i=0;
      char ch;
      do
      {
         n0= n;
         if (!isDigit(s.peek()))
         {
            ch= s.read(); ++n;
            if (isUpperCase(ch))
            {
               uint8_t t= idxMatch(ch, "STCDFHOR");
               if (t >= 0)
               { 
                  if (t < 4) { f[1]= t | 0x4; } // waveform
                  else { f[0]|= 1<<t; } // function,hold,on/off,reset
                  ++i;
                  s.print("SC:"); s.print(ch); s.print(" t:"); s.print(t); s.print(" f:"); s.print(f[0], HEX); s.println(f[1], HEX);
               }
            } else if ('?' == ch) { help(s); }
         }
      } while ((n < a) && (n > n0));
      return swapHiLo4U8(i);
   } // scanC
   
   int8_t setRS1 (char rs[], const uint8_t f1)
   {
      int8_t i= 0;
      if (f1 & 0xF0)
      {
         if (f1 & 0x80) { rs[i++]= 'R'; }
         if (f1 & 0x40) { rs[i++]= 'O'; }
         if (f1 & 0x20) { rs[i++]= 'H'; }
         if (f1 & 0x10) { rs[i++]= 'F'; }
      }
      return(i);
   } // setRS1
   int8_t setRS2 (char rs[], const uint8_t w)
   {
      int8_t i= 0;
      if (w & 0x04)
      {
static const char cc[]="STCD";
         rs[i++]= cc[w & 0x3];
      }
      return(i);
   } // setRS2
   
public:
   StreamCmd () { ; }
  
   uint8_t read (CmdSeg& cs, Stream& s)
   {
      a= avail(s);
    
      if (a > 0)
      {
         n= 0; // Expect number sequence, command flags before and/or after
         cs.nFV+= scanC(cs.cmdF, s);
         cs.nFV+= scanV(cs.v, cs.sep, SCI_VAL_MAX, s);
         cs.nFV+= scanC(cs.cmdF, s);
         //s.print("cmdF:"); s.println(cs.cmdF[0],HEX);
         resid(a-n);
      }
      uint8_t r= n; n= 0;
      return(r);
   } // read
  
   void respond (CmdSeg& cs, Stream& s)
   {
      char rs[8];
      int8_t i;
      
      i= setRS1(rs, cs.cmdR[0]);
      i+= setRS2(rs+i, cs.cmdR[1]);
      if (i > 0) { rs[i]= 0; s.print(rs); s.println(" OK"); }
      
      i= setRS1(rs, cs.cmdR[0] ^ cs.cmdF[0]);
      i+= setRS2(rs+i, cs.cmdR[1] ^ cs.cmdF[1]);
      if (i > 0) { rs[i]= 0; s.print(rs); s.println(" ERR"); }
   } // respond
}; // StreamCmd

#endif // DA_STRM_CMD_HPP
