// Duino/Common/CDS1302.hpp - classes for Maxim/Dallas DS1302 RTC with 3-wire interface
// https://github.com/DrAl-HFS/Duino.git
// Licence: AGPL3
// (c) Project Contributors June 2023

#ifndef CDS1302_HPP
#define CDS1302_HPP

#ifndef HMD3W
#include "CMD3W_BB.hpp"
CMD3W_BB gMD3W;
#define HMD3W gMD3W
#endif // HMD3W

#include "DS13xxCommon.hpp"

namespace DS1302HW
{
   enum Reg : uint8_t {
      D_DD=0x03, D_MM, // day, month
      DOW, D_YY,       // day of week, year "
      WP=0x07, TCR=0x08,   // write-protect, trickle charge,
      BURST=0x1F, RAM=0x20 // burst mode, user RAM byte
   };
   enum Cmd : uint8_t { CMD=0x80, MADDR=0x7E, RNW=0x01 }; // command byte prefix, address mask, read-not-write flag
   enum TCS : uint8_t { // trickle charge settings
      ENABLE=0xA0, DISABLE=0x50,
		  RM=0x03, R2K=0x01, R4K=0x02, R8K=0x03, // resistors
		  DM=0x0C, D1=0x04, D2=0x08		 					 // diodes
	 };

   uint8_t cmdWrite (uint8_t reg) { return( CMD | (MADDR & (reg<<1)) ); }
   uint8_t cmdRead (uint8_t reg) { return( cmdWrite(reg) | RNW ); }

   //uint8_t cmdRead (DS13xxHW::Reg r) { return( CMD | (r<<1) | RNW ); }
   //uint8_t cmdWrite (DS13xxHW::Reg r) { return( CMD | (r<<1) ); }

}; // namespace DS1307HW

class CDS1302 //: public CMD3W_BB
{
public:
   CDS1302 (void) { ; }

   // Read BCD bytes, changing order to get: YY,MM,DD,hh,mm,ss,DOW.
   // Additional non-BCD registers (WP & TCR) may also be appended.
   void readBCD (uint8_t b[], const int8_t n)
   {
      const int8_t ir= min( DS1302HW::DOW, n-1 );
      HMD3W.beginTransmission(DS1302HW::cmdRead(DS1302HW::BURST));
      for (int8_t i= ir; i>=0; i--) { b[i]= HMD3W.read(); }
      if (n >= 6)
      {
         if (n >= 7) { b[6]= b[0]; } // dow -> end (else discard)
         b[0]= HMD3W.read(); // overwrite dow with year
         for (int8_t i= 7; i < n; i++) { b[i]= HMD3W.read(); }
      }
      HMD3W.endTransmission();
    } // readBCD

   // write input (YY,MM,DD,hh,mm,ss,DOW) in hardware order (ss..MM,DOW,YY)
   void writeBCD (uint8_t b[7]) // * untested *
   {
      HMD3W.beginTransmission(DS1302HW::cmdWrite(DS1302HW::BURST));
      for (int8_t i= 5; i>0; i--) { HMD3W.write(b[i]); }
      HMD3W.write(b[6]); // dow
      HMD3W.write(b[0]); // year
      HMD3W.endTransmission();
    } // readBCD

}; // CDS1302

class CDS1302A : public CDS1302
{
public:
   CDS1302A (void) { ; }
   
   void printDateTimeBCD (Stream& s, const char end='\n')
   {
      uint8_t dt[6];
      readBCD(dt,sizeof(dt));
      printDateBCD(s, dt+0, ' ');
      printTimeBCD(s, dt+3, end);
   } // printDateTimeBCD

}; // class CDS1302A

#endif // CDS1302_HPP
