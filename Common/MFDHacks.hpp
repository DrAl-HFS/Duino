// Duino/Common/AVR/MFDHacks.hpp - Compact File (data dump) experiments
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Mar 2022

#ifndef MFD_HACKS_HPP
#define MFD_HACKS_HPP

#include "SWCRC.hpp"


// Compact File Chunk declarations: a "simple" DIY approach to reliable, flexible and
// efficient data storage (log, archive etc.) on flash memory devices having capacity
// in the 8M~1Gbit range.
// Size of a fragment payload is expected to be block-header= 65536-8 = 65528 bytes
// or less. So file size is limited to 65528*4095= 255.9 MBytes.

// DEPRECATE : better design needed
extern "C" {

// Header storage byte layout (msb to lsb)
// *Hdr0 is the general wrapper to allow simple chunk scanning
// 1011iiii jjjjjjjj jjjjjjjj kxxxrrrr = 0xBIJJJJXR
// "i" bits are chunk id code : 0x7 for [Hdr0][Hdr1][Payload], 0x8 for [Hdr0][Lnk1]
// "j" l+h form misaligned LE uint16_t object id
// "k" is the payload checksum flag (inverted ie. 0=true)
// "x" are extension flag bits (reserved, default to 0x7)
// "r" are CRC4 computed over the preceding 28 bits
// Object id=0 is reserved, so valid range 0x0001..0xFFFF
typedef union { uint32_t w; struct { uint8_t hi, jl, jh, xr; } s; } UCFCHdr0;
//---
// Immediately following *Hdr0, *Hdr1 describes an object chunk/fragment
// ssssssss ssssssss ffffffff ffffrrrr = 0xSSSSFFFR
// "s" bits are chunk payload size in bytes (l+h forms LE uint16_t) or 0xFFFF where no payload exists
// "f" l+h form LE "uint12" fragment id
// "r" are CRC4 computed over the preceding 28 bits
// Chunk id=0 is reserved for the file object descriptor, 0x001..0xFFF are data chunks
typedef union { uint32_t w; struct { uint8_t sl, sh, fl, fhr; } s; } UCFCHdr1;
//---
// Immediately following *Hdr0, *Lnk1 describes a fragment link, so that a gap in the 
// fragment ID sequence may be efficiently skipped. This simplifies truncation and may
// help in the efficient implementation of circular log buffer storage.
// tttttttt ttttxxxx ffffffff ffffrrrr = 0xTTTXFFFR
// "t" gives the target fragment ID
// "x" are extension flag bits (reserved, default to 0xF)
// "f" is the sought/expected ID for a deleted chunk (fragment)
// "r" are CRC4 computed over the preceding 28 bits
typedef union { uint32_t w; struct { uint8_t tl, thx, fl, fhr; } s; } UCFCLnk1;
//---
// The file object descriptor is expected to include user-friendly features such as
// an ASCII name to associate with the object ID. Also efficiency improvements such
// as declaring reserved regions for faster search (skipping). A two byte header is
// used to identify such data: <token><length><data...> where the token byte is given
// a distinctive 4bit prefix to avoid possible confusion. The length counts data bytes
// (ie. excluding the two byte header).
// 1110tttt = 0xET
// T : 0xF = string, assume ASCII (without BOM)
//     0xE = reservation: (reserved sizes encompass all headers & data ie. total footprint on disk)
//       first data byte = uunnnnnn : (1+n) * 16^(1+u) bytes (1+n) << ((1+u) << 2)
//       "u" is size unit (16,256,4K,64K bytes),
//       "n" is number of units (<=64, zero -> one)
// e.g. a file might appear as
//    [Hdr0][Hdr1:J1,F0][0xEF,0x07,"log.dat"][0xEE,0x01,0x48]  4+4+9+3=20bytes
//    [Hdr0][Hdr1:J1,F1][DataChunk1]
//    [Hdr0][Hdr1:J1,F2][DataChunk2]
}; // extern "C"

// ??? Nano File Chunk declarations for sub Mbit capacity devices (e.g. EEPROM) ???
// 10iissss ssssrrrr
// ooooooff ffffrrrr
// ttttttff ffffrrrr

//struct ObjHdr { uint8_t tid, obj, size; }
//struct DataChunkHdr { uint8_t tid, obj, chunk, size; }
//char hackCh (char ch) { if ((0==ch) || (ch >= ' ')) return(ch); else return('?'); }

//---

#endif // MFD_HACKS_HPP
