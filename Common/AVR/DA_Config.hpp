// Duino/Common/AVR/DA_SPIMHW.hpp - Arduino-AVR SPI Master Hardware wrapper
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Dec 2020 - Mar 2021

#ifndef DA_CONFIG_HPP
#define DA_CONFIG_HPP

#include <EEPROM.h>


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
static const uint8_t CRC4::kPoly[16]= {
   0x0, 0x7, 0xe, 0x9, 0xb, 0xc, 0x5, 0x2,
   0x1, 0x6, 0xf, 0x8, 0xa, 0xd, 0x4, 0x3
};

// Compact File Chunk declarations: a DIY approach to simple and efficient
// yet reasonably flexible data storage (log, archive etc.) on flash memory
// devices having capacity in the 1~256Mbit range.
extern "C" {
// Header storage byte layout (msb to lsb)
// *Hdr0 is the general wrapper to allow simple chunk scanning
// 1011iiii ssssssss ssssssss xxxxrrrr = 0xBISSSSXR
// "i" bits are header id / chunk size code : 0xF for [Hdr0][Hdr1], 0xE for [Hdr0][Lnk1]
// "s" bits are chunk payload size in bytes (l+h forms misaligned LE uint16_t)
// "x" are extension flag bits (reserved, default to 0xF)
// "r" are CRC4 computed over the preceding 28 bits
typedef union { uint32_t w; struct { uint8_t hi, sl, sh, xc; } s; } UCFCHdr0;
//---
// Immediately following *Hdr0, *Hdr1 describes an object fragment
// jjjjjjjj jjjjjjjj ffffffff ffffrrrr = 0xJJJJFFFR
// "j" l+h form LE uint16_t object id
// "f" l+ form LE "uint12" fragment/chunk id
// "r" are CRC4 computed over the preceding 28 bits
// Object id=0 is reserved, so valid range 0x0001..0xFFFF
// Fragment id=0 is reserved for the file object descriptor, 0x001..0xFFF are data chunks
typedef union { uint32_t w; struct { uint8_t jl, jh, fl, fc; } s; } UCFCHdr1;
//---
// Immediately following *Hdr0, *Lnk1 describes a fragment link, so that a gap in the 
// fragment ID sequence may be efficiently skipped. This simplifies truncation and may
// help in the efficient implementation of circular log buffer storage.
// tttttttt ttttxxxx ffffffff ffffrrrr = 0xTTTXFFFR
// "t" gives the target fragment ID
// "x" are extension flag bits (reserved, default to 0xF)
// "f" is the sought/expected ID for a deleted fragment
// "r" are CRC4 computed over the preceding 28 bits
typedef union { uint32_t w; struct { uint8_t tl, thx, fl, fc; } s; } UCFCLnk1;
//---
// The file object descriptor is expected to include user-friendly features such as
// an ASCII name to associate with the object ID.
// e.g. a file might appear as
//    [Hdr0][Hdr1:J1,F0]["log.dat"]
//    ..
//    [Hdr0][Hdr1:J1,F1][DataChunk1]
//    ..
//    [Hdr0][Hdr1:J1,F2][DataChunk2]
};

// Micro File Chunk declarations for sub Mbit capacity devices (e.g. EEPROM)
// 10iissss ssssrrrr
// ooooooff ffffrrrr
// ttttttff ffffrrrr

//struct ObjHdr { uint8_t tid, obj, size; }
//struct DataChunkHdr { uint8_t tid, obj, chunk, size; }
//char hackCh (char ch) { if ((0==ch) || (ch >= ' ')) return(ch); else return('?'); }

//---

// TODO : wrap as ID class
int8_t setID (char idzs[])
{
   int8_t i=0;
   if (EEPROM.read(i) != idzs[i])
   {
      EEPROM.write(i,idzs[i]);
      if (0 != idzs[i++])
      {
         do
         {
            EEPROM.update(i,idzs[i]);
            //if (EEPROM.read(i) != idzs[i]) { EEPROM.write(i,idzs[i]); }
         } while (0 != idzs[i++]);
      }
   }
   return(i);
} // setID

int8_t getID (char idzs[])
{
   int8_t i=0;
   do
   {
      idzs[i]= EEPROM.read(i);
   } while (0 != idzs[i++]);
   return(i);
} // getID

#include <avr/boot.h>
void sig (Stream& s)  // looks like garbage...
{ // avrdude: Device signature = 0x1e950f (probably m328p)
   char lot[7];
   s.print("sig:");
   for (uint8_t i= 0x0E; i < (6+0x0E); i++)
   { // NB - bizarre endian reversal
      lot[(i-0x0E)^0x01]= boot_signature_byte_get(i);
   }
   lot[6]= 0;
   s.print("lot:");
   s.print(lot);
   s.print(" #");
   s.print(boot_signature_byte_get(0x15)); // W
   s.print("(");
   s.print(boot_signature_byte_get(0x17)); // X
   s.print(",");
   s.print(boot_signature_byte_get(0x16)); // Y
   s.println(")");
   s.flush();
   for (uint8_t i=0; i<0xE; i++)
   { // still doesn't appear to capture everything...
      uint8_t b= boot_signature_byte_get(i);
      if (0xFF == b) { s.println(b,HEX); }
      else { s.print(b,HEX); }
      s.flush();
   }
   // Cal. @ 0x01,03,05 -> 9A FF 4F, (RCOSC, TSOFFSET, TSGAIN)
   // 1E 9A 95 FF FC 4F F2 6F
   // F9 FF 17 FF FF 59 36 31
   // 34 38 37 FF 13 1A 13 17
   // 11 28 13 8F FF F
} // sig


#endif // DA_CONFIG_HPP
