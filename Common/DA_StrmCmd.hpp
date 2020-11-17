// Duino/Common/DA_SerCmd.h - Arduino (AVR) specific serial command processing
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Nov 2020

#ifndef DA_STRM_CMD_H
#define DA_STRM_CMD_H

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
   s.println("*");
   s.println("*\tS T C D - waveform: Sin,Tri,Clock,Double");
   s.println("*\tF - Function (cyclic)");
   s.println("*\tH O R - set: Hold,On/Off,Reset");
   s.println("*\t? - help (this text)");
   s.println("*");
} // help

class StreamCmd : StreamFilter
{
protected:
   uint8_t n, a; // crap hack
  
   uint8_t scanV (USciExp v[], const uint8_t max, Stream& s)
   {
      uint8_t n0, i=0;
      char ch;
      do // Read a sequence of separated numbers
      {  
         n0= n;
         n+= v[i].readStream(s,a);
         i+= (n > n0);
         do
         {
            ch= idxMatch(s.peek(), ",;:"); // space? tab ?
            if (ch >= 0)
            {
               ch= s.read(); n++; 
               //if (i < SEP_CH_MAX) { cs.sep[i]= ch; }
            }
         } while (ch > 0);
      } while ((n < a) && (n > n0) && (i < max));
      return(i);
   } // scanV

   uint8_t scanC (uint8_t& f, Stream& s)
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
                  if (t < 4) { f= (f & 0xF8) | (1+t); } // waveform
                  else { f|= 1<<t; } // function,hold,on/off,reset
                  ++i;
                  //s.print("SC:"); s.print(ch); s.print(" t:"); s.print(t); s.print(" f:"); s.println(f, HEX);
               }
            } else if ('?' == ch) { help(s); }
         }
      } while ((n < a) && (n > n0));
      return swapHiLo4U8(i);
   } // scanC
   
public:
   StreamCmd () { ; }
  
   uint8_t read (CmdSeg& cs, Stream& s)
   {
      a= avail(s);
    
      if (a > 0)
      {
         n= 0; // Expect number sequence, command flags before and/or after
         cs.nFV+= scanC(cs.cmdF[0], s);
         cs.nFV+= scanV(cs.v, SCI_VAL_MAX, s);
         cs.nFV+= scanC(cs.cmdF[0], s);
         //s.print("cmdF:"); s.println(cs.cmdF[0],HEX);
         resid(a-n);
      }
      uint8_t r= n; n= 0;
      return(r);
   } // StreamCmd::read
  
}; // StreamCmd

#endif // DA_STRM_CMD_H
