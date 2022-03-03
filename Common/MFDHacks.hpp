// Duino/Common/AVR/SWCRC.hpp - Software CRC routines
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Mar 2022

#ifndef MFD_HACKS_HPP
#define MFD_HACKS_HPP

#include "SWCRC.hpp"


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

#endif // MFD_HACKS_HPP
