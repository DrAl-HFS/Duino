// Duino/Common/AVR/MFDHacks.hpp - Mini File Dump (Compact File Chunk) experiments
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Mar 2022

#ifndef MFD_HACKS_HPP
#define MFD_HACKS_HPP

#include "SWCRC.hpp"


// Compact File Chunk declarations: a "simple" DIY approach to reliable, flexible and
// efficient data storage (log, archive etc.) on flash memory devices having capacity
// in the 8M~1Gbit range. This is partly an exercise in learning the fundamentals of
// file system design, specifically the code size & complexity vs. usability tradeoffs
// involved.

extern "C" {

// A file consists of an object split into chunks. Each chunk is identified by an object
// header containing the object ID and declaring how much storage is reserved for that
// chunk. A chunk contains a set of fragments, ordered by fragment ID. Appending to a file
// requires creating a new fragment, either within the available reserved storage of an
// existing chunk or by creating a new chunk (with object header) in free space. Reading
// a file requires retreiving its fragments in the order given by the fragment ID sequence.
// A "last fragment" marker is needed to terminate the fragment search process but this
// marker has to be easily deleted when a new fragment is appended. A redirect is used for
// this purpose: this contains the next expected fragment ID and gives a target fragment ID
// of zero (F0, the first possible fragment ID). Fragment zero is special: it does not contain
// user data but rather attribute information such as the file name. Thus a file created
// empty would contain fragment zero followed by a redirect F1->F0, marking the end of file.
// (Thus fragment ID F0 is equivalent to NULL.)

// A generic chunk micro header & footer allow simple scanning with error checking
// The object micro-payload includes a storage reservation for more efficient allocation and search.
// 1011ssss xxxxiiii [micro-payload] rrrrrrrr ssss1101 = 0xBSIX ... RRSD
// "s" bits give number of bytes to skip between end of header to start of footer, coding TBD.
// "x" are extension bits (reserved, default to 0xF) CONSIDER: sequence number?
// "i" bits are chunk id code 
// DEPRECATE: "q" are CRC4 computed over i,x
// "r" are CRC8 computed over the header (?) & payload bytes
// header & size bits are repeated (swapped) at end of footer to further help identify errors
typedef struct { uint8_t hs, xi; } CFCUH; // micro header
typedef struct { uint8_t rr, sh; } CFCUF; // micro footer
// 3 header payload types:
// Object with storage reservation (allocated)
// jjjjjjjj jjjjjjjj uvvvvvvv
// "j" bits are object ID l+h (LE uint16_t)
// "u" is size unit: 1=page (256Byte), 0= block (64kByte)
// "v" is reservation count (number of size units measured from start of enclosing unit, no 
// other valid object permitted within this region)
typedef struct { uint8_t jl, jh, uv; } CFCJ0; // Data obJect, reservation (unit & value)

// Data fragment header ID & size, both LE uint16_t, extension flags eg. data payload CRC32
// ffffffff ffffffff ssssssss ssssssss xxxxxxxx
typedef struct { uint8_t fl, fh, sl, sh, xx; } CFCD0; // Data payload fRagment-ID & size

// Redirect an expected fragment-ID -> target-ID
// ffffffff ffffffff tttttttt tttttttt
typedef struct { uint8_t fl, fh, tl, th; } CFCR0;

// Complete headers (packed)
typedef struct { CFCUH h; CFCJ0 j; CFCUF f; } CFCHdrJ0;
typedef struct { CFCUH h; CFCD0 d; CFCUF f; } CFCHdrD0;
typedef struct { CFCUH h; CFCR0 r; CFCUF f; } CFCHdrR0;
// Object header precedes one or more fragments and/or redirects
// (all expected to lie within reserved storage).

// DEPRECATED : initial (inflexible, weak) design.
// Size of a fragment payload is expected to be block-header= 65536-8 = 65528 bytes
// or less. So file size is limited to 65528*4095= 255.9 MBytes.

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

#define MASKB(b) ((1<<(b))-1)
// (right shift) truncate and round up by bits
#define TRNRUB(x,b) (((x) >> b) + (((x) & MASKB(b)) > 0))

class MFDMicro: public CRC8
{
protected:
   struct ValInf { uint8_t s, crc; };
#if 0
   ValInf vi;

   void logVI (Stream& s)
   {
      s.print("vi:"); s.print(vi.s); s.print(','); s.print(vi.emi); s.print(','); 
      s.print(vi.crc,HEX); s.print(','); s.print(vi.l0); s.print(','); s.println(vi.l1);
   } // logVI s.println();
#endif

   uint8_t encodeRV (uint32_t b)
   {
      if ((b > 0) && (b <= (1<<22))) // allow up to 4MB (25% of 128Mbit)
      {
         uint16_t maxP= TRNRUB(b, 8); // bytes -> pages
         if (maxP > 0x7F) { return(TRNRUB(maxP, 8) & 0x7F); } // pages -> 64K blocks
         else { return( 0x80 | maxP); } // set page flag
      }
      return(0);
   } // encodeRV

   uint32_t genMicroHdr (uint8_t uh[2], const uint8_t t, const uint8_t x=0xF)
   {
      uint32_t i= 0;
      if (t <= 0xF)
      {
         uh[i++]= 0xB0; // start marker followed by micro-payload size (deferred)
         uh[i++]= (x << 4) | t; // extension bits, micro header type
      }
      return(i);
   } // genMicroHdr

   uint32_t genMicroFtr (uint8_t h[], uint32_t i)
   {
      if (i >= sizeof(CFCUH))
      {
         uint8_t s= i-sizeof(CFCUH); // deduct header
         if ((0xB0 == h[0]) && (s <= 0xF))
         {
            h[0]|= s; // fix size in micro-header
            h[i]= CRC8::compute(h+0,i); // NB: no increment! (Zealous compiler optimisation hazard.)
            h[++i]= (s << 4) | 0xD; // size again followed by end marker
            return(++i);
         }
      }
      return(0);
   } // genMicroFtr

public:
   MFDMicro (void) { ; }
   
   uint32_t validate (const uint8_t b[], const uint32_t n)
   {
      ValInf vi;
      if (n >= 4)
      {
         vi.s= b[0] & 0x0F;
         if (vi.s < (n-4))
         {
            const uint8_t i= sizeof(CFCUH) + vi.s + sizeof(CFCUF)-1; // index of end mark
            if (  ((b[i] >> 4) == vi.s) &&
                  (0xB0 == (b[0] & 0xF0)) &&
                  (0x0D == (b[i] & 0x0F))   )
            {
               vi.crc= CRC8::compute(b, sizeof(CFCUH)+vi.s);
               if (b[i-1] == vi.crc)   // CRC precedes end mark
               {
                  return(vi.s + sizeof(CFCUH) + sizeof(CFCUF));
               }
            }
         }
      }
      return(0);
   } // validate
   
}; // class MFDMicro

class MFDHeader : public MFDMicro
{
protected:

   uint32_t genObjHdr (uint8_t hb[7], const uint16_t id, const uint8_t rv=0)
   {
      uint32_t i= genMicroHdr(hb,0xF);

      hb[i++]= id; // & 0xFF;
      hb[i++]= id >> 8;
      hb[i++]= rv;

      return genMicroFtr(hb,i);
   } // genObjHdr

   uint32_t genFragHdr (uint8_t hb[9], const uint16_t id, const uint16_t s)
   {
      uint32_t i= genMicroHdr(hb,0xE);

      hb[i++]= id; // & 0xFF;
      hb[i++]= id >> 8;
      hb[i++]= s; // & 0xFF;
      hb[i++]= s >> 8;
      hb[i++]= 0xFF; // dummy ext flags

      return genMicroFtr(hb,i);
    } // genFragHdr

public:
   MFDHeader (void) { ; }

   uint32_t genObjFragHdr (uint8_t hb[16], const uint16_t objID, const uint16_t fragID, const uint16_t s0, uint32_t sx=0)
   {
      uint32_t i=0, s= s0+16;
      if (s > sx) { sx= s; }
      s= genObjHdr(hb, objID, encodeRV(sx));
      if (s > 0)
      {
         i+= s; 
         s= genFragHdr(hb+i, fragID, s0);
         if (s > 0) { i+= s; }
      }
      return(i);
   } // genObjFragHdr

}; // class MFDHeader

class MFDHack : public MFDHeader
{
public:
   MFDHack (void) { ; }

   uint32_t create (const uint8_t b[], const uint32_t n, const uint16_t objID, CW25QUtil& d)
   {
      return(0);
   } // create

   uint32_t read (uint8_t b[], const uint32_t n, const uint16_t objID)
   {
      return(0);
   } // read

   uint32_t append (const uint8_t b[], const uint32_t n, const uint16_t objID)
   {
      return(0);
   } // append

   void test (Stream& s, CW25QUtil& d)
   {
      uint8_t hb[20], l[2], n;
      
      n= genObjFragHdr(hb,1,0,4,900);
      s.print("MFD hdrs ["); s.print(n); s.print("]:");
      char f[4]="   ";
      dumpHexFmt(s,hb,n,f,1);
      s.println();
      s.print("validate:");
      l[0]= validate(hb+0,sizeof(hb));
      //logVI(s);
      if (l[0] > 0)
      {
         l[1]= validate(hb+l[0],sizeof(hb)-l[0]);
         //logVI(s);
      }
      s.print(" l[0]="); s.print(l[0]);
      s.print(" l[1]="); s.print(l[1]);
      s.println();
   } // test

}; // class MFDHack

#endif // MFD_HACKS_HPP
