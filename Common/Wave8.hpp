// Duino/Common/tmp.hpp - 8bit Waveform jiggery-pokery
// https://github.com/DrAl-HFS/Duino.git
// Licence: AGPL3
// (c) Project Contributors April 2021

#ifndef WAVE8_HPP
#define WAVE8_HPP

#include "MBD/mbdDef.h"

//#define PENTATONIC_ROOT 1.1487	// 5th root of 2
//#define HEPTATONIC_ROOT 1.1041	// 7th root of 2
#define DODECATONIC_ROOT 1.059	// 12th root of 2 - basis of the even tempered scale
#define DDT0 1 // fundamental
#define DDT1 DODECATONIC_ROOT
#define DDT2 DDT1*DODECATONIC_ROOT
#define DDT3 DDT2*DODECATONIC_ROOT
#define DDT4 DDT3*DODECATONIC_ROOT
#define DDT5 DDT4*DODECATONIC_ROOT
#define DDT6 DDT5*DODECATONIC_ROOT
#define DDT7 DDT6*DODECATONIC_ROOT
#define DDT8 DDT7*DODECATONIC_ROOT
#define DDT9 DDT8*DODECATONIC_ROOT
#define DDT10 DDT9*DODECATONIC_ROOT
#define DDT11 DDT10*DODECATONIC_ROOT
#define DDT12 DDT11*DODECATONIC_ROOT
#define DDT13 DDT12*DODECATONIC_ROOT
#define DDT14 DDT13*DODECATONIC_ROOT
//#define DDT12 2 // octave

#define DAC_RATE			10000	// 10kHz
#define WAVE_SAMPLES	128

#define FM 110*0x100
static const U16 evenTempScale[]=
{ FM, FM*DDT1, FM*DDT2, FM*DDT3, FM*DDT4, FM*DDT5, FM*DDT6, 
	FM*DDT7, FM*DDT8, FM*DDT9, FM*DDT10, FM*DDT11, FM*DDT12, 
	FM*DDT13, FM*DDT14 };

// Quarter sine table: 32 * 4 = 128 sample values generated
static const int8_t qSinQ7M5[32]=
{
	0x00, 0x06, 0x0C, 0x13, 0x19, 0x1F, 0x26, 0x2C,
	0x32, 0x37, 0x3D, 0x43, 0x48, 0x4D, 0x52, 0x57,
	0x5C, 0x60, 0x64, 0x68, 0x6B, 0x6F, 0x72, 0x74,
	0x77, 0x79, 0x7A, 0x7C, 0x7D, 0x7E, 0x7E, 0x7F
};

class CMiniLUT8
{
protected:
	 uint8_t idxM (const uint8_t i)
	 {
      uint8_t j= i & 0x1F;
      if (i & 0x20) { j= 0x1F - j; } // mirror index
      return(j);
   } // idxM
   uint8_t toU8 (int8_t x) { return(0x7F+x); }
   
public:
   CMiniLUT8 (void) { ; }

   int8_t sampleMF (const uint8_t i)
   {
      int8_t x= qSinQ7M5[ idxM(i) ];
      if (i & 0x40) { x= -x; } // flip sign
      return(x);
   }
   uint8_t sampleU8 (uint8_t i) { return toU8(sampleMF(i)); }
   
   uint8_t filterSampleU8 (const UU16 t8Q8)	// CAVEAT : little-endian dependancy!
   {	// rudimentry 2-point (linear) reconstruction filter
			int8_t s[2];
			s[0]= sampleMF(t8Q8.u8[1]);
			if (0 == t8Q8.u8[0]) { return toU8(s[0]); }
			//else
			s[1]= sampleMF(t8Q8.u8[1]+1);
			//int16_t wq8= (0x100 - r) * s[0] + r * s[1];
			int16_t i7q8= t8Q8.u8[0] * (s[1] - s[0]) + (0x100 * s[0]); // I7Q8_LERP(a,b,r)
			return toU8(i7q8 / 0x100);
   }
}; // CMiniLUT8

uint8_t triangle (const uint8_t i)
{
   int8_t j= i & 0x3F;
   if (i & 0x40) { j= 0x3F - j; } // mirror index
   return(j<<2);
} // triangle

class CSampleCtrl
{
public:
	UU16 iQ8;
	uint16_t rQ8;
	
	CSampleCtrl (uint32_t r= 0x100) { set(r); }

	void set (uint32_t r) { rQ8= (r * WAVE_SAMPLES) / DAC_RATE; }
	void step (void) { iQ8.u16+= rQ8; }
	
	//uint8_t get (uint8_t& r) { r= i.u8[0]; return(i.u8[1]); } 
}; // CWave8

#endif // WAVE8_HPP
