// Duino/Common/AVR/SWCRC.hpp - Software CRC routines
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Mar 2022

#ifndef SWCRC_HPP
#define SWCRC_HPP


/***/

//--- Hacking ... ideas on more robust data dumping, using log-file-structures ?

// adapted from linux/lib/crc4.h
class CRC4
{
static const uint8_t kPoly[16];

   // MB: 0xF mask applied AFTER xor : this protects against error where input exceeds 0xF
   uint8_t compute4bit (const uint8_t c4, const uint8_t i4) const { return kPoly[ (c4 ^ i4) & 0xF ]; }

public:
   CRC4 (void) { ; }

   // NB: effectiveness relies on input <= (2^(4+1))-1 = 31bits (almost 4 bytes)
   uint8_t compute (const uint8_t b[], const int8_t n, uint8_t c= 0xF) const
   {
      for (int8_t i= 0; i<n; i++)
      {  // process nybbles lsb first
         c= compute4bit(c, b[i]); // & 0xF); redundant mask
         c= compute4bit(c, b[i]>>4);
      }
      return(c);
   } // compute
}; // CRC4
// Polynomial lookup might be compacted 50%
// NB: 'static' keyword appears only in declaration! 
const uint8_t CRC4::kPoly[16]= {
   0x0, 0x7, 0xe, 0x9, 0xb, 0xc, 0x5, 0x2,
   0x1, 0x6, 0xf, 0x8, 0xa, 0xd, 0x4, 0x3
};

#endif // SWCRC_HPP
