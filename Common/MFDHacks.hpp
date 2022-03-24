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
// of zero (F0, the first possible fragment ID). Fragment zero is special: it typically does
// not contain user data but rather attribute information such as the file name. Thus a file
// created empty would contain fragment zero followed by a redirect F1->F0, marking the end
// of file. (Thus fragment ID F0 is equivalent to NULL.)
// Any fragment may contain attribute information, signalled by a flag in the fragment header.
// When signalled, the fragment *must* begin with an attribute token. For a regular data
// fragment this typically would be an encapsulation (255 bytes max) so that multiple attribute
// tokens can be unambiguously discriminated from data bytes.

// A generic chunk micro header & footer allow simple scanning with error checking
// The object micro-payload includes a storage reservation for more efficient allocation and search.
// 1011ssss xxxxiiii [micro-payload] rrrrrrrr ssss1101 = 0xBSIX ... RRSD
// "s" bits give number of bytes to skip between end of header to start of footer, coding TBD.
// "x" are extension bits (reserved, default to 0xF) CONSIDER: sequence number?
// "i" bits are chunk id code
// "r" are CRC8 computed over the header (?) & payload bytes (footer is excluded)
// header & size bits are repeated (swapped) at end of footer to further help identify errors
typedef struct { uint8_t hs, xi; } CFCUH; // micro header
typedef struct { uint8_t rr, sh; } CFCUF; // micro footer

#define HDR_UHF_BYTES (sizeof(CFCUH)+sizeof(CFCUF))

// 3 header payload types:
// Object with storage reservation (allocated)
// jjjjjjjj jjjjjjjj uvvvvvvv
// "j" bits are object ID l+h (LE uint16_t)
// "u" is size unit: 1=page (256Byte), 0= block (64kByte)
// "v" is reservation count (number of size units measured from start of enclosing unit, no
// other valid object permitted within this region)
typedef struct { uint8_t jl, jh, uv; } CFCJ0; // Data obJect, reservation (unit & value)

// Data fragment header ID & size, both LE uint16_t, extension flags eg. data payload CRC presence&type
// ffffffff ffffffff ssssssss ssssssss xxxxxxxx
typedef struct { uint8_t fl, fh, sl, sh, xx; } CFCD0; // Data payload fragment-ID & size

// Redirect an expected fragment-ID -> target-ID
// ffffffff ffffffff tttttttt tttttttt
typedef struct { uint8_t fl, fh, tl, th; } CFCR0;

// Complete headers (packed)
typedef struct { CFCUH h; CFCJ0 j; CFCUF f; } CFCHdrJ0;
typedef struct { CFCUH h; CFCD0 d; CFCUF f; } CFCHdrD0;
typedef struct { CFCUH h; CFCR0 r; CFCUF f; } CFCHdrR0;
// Object header precedes one or more fragments and/or redirects
// (all expected to lie within reserved storage).

// Token 1110tttt = 0xET,XX where XX bits are typically a byte count for subsequent data
typedef union { uint8_t a[2]; struct { uint8_t ht, xx; }; } CFCTok0;

#define HDR_OJDF_BYTES (sizeof(CFCHdrJ0)+sizeof(CFCHdrD0))

// The file object descriptor (F0) is expected to include user-friendly features such as
// an ASCII name to associate with the object ID. A two byte header is used to identify
// such data: <token><length><data...> where the token byte is given a distinctive 4bit
// prefix to avoid possible confusion. The length counts data bytes (ie. excluding the
// two byte header).
// 1110tttt = 0xET where T is:
//     0x0 .. 0x9 = reserved values, treat as "ignore"
//     0xA = character string, assume ASCII (without BOM)
//     0xB = bcd4 string
//     0xC = CRC8/16/32 *special arg format*
//     0xD = ???
//     0xE = encapsulation, used within data bearing fragments to avoid ambiguity
//     0xF = reserved value, treat as "ignore"
// e.g. a file might appear as
//    [HdrJ:J1][HdrD:F0,18][0xEF,0x07,"log.dat"][0xEE,0x03,0x11,0x28,0x31]  7+9+9+5=30bytes
//    [HdrD:F1][DataChunk1]
//    [HdrJ:J1][HdrD:F2][DataChunk2]

// CRC: <token><arg><crc> = 0xEC,<kinnnnnn>,<rrrrrrrr>
//    k : 0->CRC8, 1->CRC32
//    i : 0->preceding, 1->succeeding (direction of data on which calculated).
//          Calculation always excludes <crc> bytes but includes <token><arg>
//          (with implicit pre/post directional zero padding for CRC32) 
//    nnnnnn : 1..63 -> number of (1 or 4 byte) elements in addition to <token><arg> or 0->all in scope (encapsulation/fragment).

//--------------------------------------------
// *** DEPRECATED : unsatisfactory design.
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
// DEPRECATED ***


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

   int genMicroHdr (uint8_t uh[2], const uint8_t t, const uint8_t x=0xF)
   {
      uint32_t i= 0;
      if (t <= 0xF)
      {
         uh[i++]= 0xB0; // start marker followed by micro-payload size (deferred)
         uh[i++]= (x << 4) | t; // extension bits, micro header type
      }
      return(i);
   } // genMicroHdr

   int genMicroFtr (uint8_t h[], uint32_t i)
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

   uint32_t decodeRV (uint8_t rv)
   {
      uint32_t b= rv & 0x7F;
      if (rv & 0x80) { return(b * 0x100); } // 256Byte pages
      //else
      return(b * 0x10000); // 64K blocks
   } // decodeRV

   int validate (const uint8_t b[], const int n, const CFCUH **pp=NULL)
   {
      ValInf vi;
      if ((n > HDR_UHF_BYTES) &&    // min size (reject no payload) and
         (0xB0 == (b[0] & 0xF0)))   // start mark
      {
         vi.s= b[0] & 0x0F; // size
         if ((vi.s+HDR_UHF_BYTES) <= n) // ensure complete
         {
            const uint8_t i= vi.s + HDR_UHF_BYTES - 1; // index of end mark
            if ( ((b[i] >> 4) == vi.s) &&   // match size and
                 (0x0D == (b[i] & 0x0F)) )  // end code
            {  // now verify CRC
               vi.crc= CRC8::compute(b, sizeof(CFCUH)+vi.s);
               if (b[i-1] == vi.crc)   // CRC precedes end mark
               {
                  if (pp) { *pp= (const CFCUH*)b; }
                  return(vi.s);
               }
               return(-vi.s); // bad CRC
            }
         }
      }
      return(0);
   } // validate

}; // class MFDMicro

class MFDHeader : public MFDMicro
{
protected:

   int genObjHdr (uint8_t hb[7], const uint16_t id, const uint8_t rv=0)
   {
      int i= genMicroHdr(hb,0xF);

      hb[i++]= id; // & 0xFF;
      hb[i++]= id >> 8;
      hb[i++]= rv;

      return genMicroFtr(hb,i);
   } // genObjHdr

   int genFragHdr (uint8_t hb[9], const uint16_t id, const uint16_t s, const uint8_t extf=0xFF)
   {
      int i= genMicroHdr(hb,0xE);

      hb[i++]= id; // & 0xFF;
      hb[i++]= id >> 8;
      hb[i++]= s; // & 0xFF;
      hb[i++]= s >> 8;
      hb[i++]= extf; // extension flags

      return genMicroFtr(hb,i);
    } // genFragHdr

public:
   MFDHeader (void) { ; }

   int genObjFragHdr (uint8_t hb[HDR_OJDF_BYTES], const uint16_t objID, const uint16_t fragID, const uint16_t s0, const uint32_t usx=0)
   {
      int i=0, s= s0 + HDR_OJDF_BYTES;
      s= genObjHdr(hb, objID, encodeRV(s+usx));
      if (s > 0)
      {
         i+= s;
         s= genFragHdr(hb+i, fragID, s0);
         if (s > 0) { i+= s; }
      }
      return(i);
   } // genObjFragHdr

   int dissect (const uint8_t b[], const int n, Stream& s)
   {
      const CFCUH *p=NULL;
      int r= validate(b,n,&p);
      if (r > 0)
      {
         switch(p->xi & 0x0F)
         {
            case 0xF :
               if (3 == r)
               {
                  const CFCJ0 *pJ= (const CFCJ0*)(p+1);
                  s.print('J'); s.print(pJ->jl); s.print(','); s.print(decodeRV(pJ->uv)); s.print('B');
               }
               break;
            case 0xE :
               if (5 == r)
               {
                  const CFCD0 *pF= (const CFCD0*)(p+1);
                  s.print('F'); s.print(pF->fl); s.print(','); s.print(pF->sl); s.print('B');
               }
               break;
            case 0xD :
               if (4 == r)
               {
                  const CFCR0 *pR= (const CFCR0*)(p+1);
                  s.print('R'); s.print(pR->fl); s.print("->"); s.print(pR->tl);
               }
               break;
            default : s.print('?');
         }
         s.print(';');
         return(r + HDR_UHF_BYTES);
      }
   }
}; // class MFDHeader

#define MAX_BB 32 // KISS
class BBuff
{
protected:
   uint8_t bb[MAX_BB], iB;

public:
   BBuff (void) : iB{0} { ; }

   void reset (void) { iB= 0; }

   int avail (void) const { return(MAX_BB - (int)iB); }
   int bytes (void) { return(iB); }

   uint8_t *claim (const int n)
   {
      uint8_t *p= NULL;
      if ((n > 0) && (avail() >= n))
      {
         p= bb+iB;
         iB+= n;
      }
      return(p);
   } // claim

   // if (i < MAX_BB)
   uint8_t& operator [] (uint8_t i) { return bb[i]; }
}; // BBuff

// TODO : find alternate name to avoid confusion with file object fragments
#define MAX_FRAG 8
class FragAsm : public BBuff
{
protected:
   const uint8_t *pF[MAX_FRAG]; // NB: pointer needed for device commit()
   uint8_t        lF[MAX_FRAG];
   uint8_t iF;    // CONSIDER: merge index & flag
   uint8_t flag;  // = whether last fragment is in BBuff

   // PARANOID: (i < MAX_FRAG) && 
   bool nonEmpty (const uint8_t i=0) const { return ((NULL != pF[i]) && (0 != lF[i])); }

   bool newFrag (const uint8_t *p, uint16_t n)
   {
      iF+= nonEmpty(iF);
      if (iF >= MAX_FRAG) { return(false); }
      //else
      pF[iF]= p; lF[iF]= n;
      return(true);
   } // newFrag

   void extend (const uint8_t n) { lF[iF]+= n; }

   struct FragPos
   {
      uint8_t iF, iB;
      FragPos (void) : iF{0}, iB{0} { ; }
   }; // struct FragPos

   bool advance (FragPos& pos, int b) const
   {
      while ((b > 0) && (pos.iF <= iF))
      {
         int rB= lF[pos.iF] - pos.iB;
         if (rB < 0) { return(false); } // error
         if (b > rB) // likliest?
         {
            b-= rB;
            pos.iF++;
            pos.iB= 0;
         }
         else if (b < rB)
         {
            pos.iB+= b;
            return(true);
         }
         else // (b == rB)
         {
            pos.iF++;
            pos.iB= 0;
            return(true);
         }
      }
      return(b <= 0);
   } // advance

public:
   FragAsm (void) { reset(); }

   void reset (void)
   {
      BBuff::reset();
      iF= 0;
      flag= 1;
      for (uint8_t i=0; i<MAX_FRAG; i++) { pF[i]= NULL; lF[i]= 0; }
   } // reset

   uint8_t *claim (const uint8_t n, const uint8_t brkbf= 0)
   {
      uint8_t *p= BBuff::claim(n);
      if (p)
      {
         if ((flag|brkbf) > 0) { newFrag(p,n); flag=0; }
         else { extend(n); } // tack on end
      }
      return(p);
   } // claim

   uint8_t append (const uint8_t *p, const uint8_t n)
   {
      //if ((NULL != p) && (n > 0) && : PARANOID
      if (newFrag(p,n)) { flag= 1; return(n); }
      //else
      return(0);
   } // add

   uint16_t sumFragBytesFI (const uint8_t i0=0) const
   {
      uint16_t s= lF[i0];
      for (uint8_t i= i0+1; i<=iF; i++) { s+= lF[i]; }
      return(s);
   } // sumFragBytesFI

   uint8_t crc8FromIdx (const uint8_t i0=0) const
   {
      CRC8 crc8;
      uint8_t c= 0xFF;
      // deduct 1 from length of last
      for (uint8_t i= i0; i<=iF; i++) { c= crc8.compute(pF[i], lF[i]-(i==iF), c); }
      return(c);
   } // crc8FromIdx

   uint8_t count (void) const { return(iF + nonEmpty(iF)); }

   int commit (UU32 addr, CW25QUtil& dev) const
   {
      return dev.dataWriteFrags(addr, pF, lF, count());
/*    FragPos pos;
      do
      {
         if (advance(pos,r))
         {
            addr.u32+= r;
            r= dev.dataWrite(pF[pos.iF]+pos.iB, lF[pos.iF]-pos.iB, addr);
         }
      } while ((r > 0) && (tB < nB));
      return(tB); */
   } // commit

}; // FragAsm

class FragAsmDbg : public FragAsm
{
protected:
   //Stream& dbg;

public:
   //FragAsmDbg (Stream& s) : dbg(s) { ; }

   void test (Stream& s, int a=16)
   {
      FragPos pos;
      bool r;
      s.print("FragAsmDbg::test() - ");
      s.print(" ["); s.print(pos.iF); s.print("],"); s.println(pos.iB); s.print(" -> ");
      if (a <= 0) { a= sumFragBytesFI()/2; }
      r= advance(pos, a);
      s.print(r); s.print("; ["); s.print(pos.iF); s.print("],"); s.println(pos.iB);
   } // test

   int dump (uint8_t b[], const int max) const // flatten
   {
      int k= 0;
      for (uint8_t i=0; i<=iF; i++)
      {
         const uint8_t n= lF[i], *p= pF[i];
         for (uint8_t j=0; j < lF[i]; j++)
         {
            b[k]= p[j];
            k+= (k<max);
         }
      }
      return(k);
   } // dump

   void dump (Stream& s) const // DEPRECATE
   {
      for (uint8_t i=0; i<=iF; i++)
      { s.print("0x"); s.print((uint32_t)(pF[i])-RAM_BASE,HEX); s.print('['); s.print(lF[i]); s.println(']'); }
      for (uint8_t i=0; i<=iF; i++)
      { dumpHexFmt(s, pF[i], lF[i]); s.print('\t'); dumpCharFmt(s, pF[i], lF[i]); s.println(); }
   }
}; // FragAsm

class MFDAsm : public MFDHeader
{
public:
   MFDAsm (void) { ; }

   int create (FragAsm& fa, const char *name, CClock *pC, const uint16_t id, const uint32_t usx)
   {
      uint8_t *pH= fa.claim(HDR_OJDF_BYTES);
      if (pH)
      {
         uint8_t brkbf=0x1; // force break at start of F0 payload to simplify totalling (otherwise first token is merged into header)
         //s.print("fa.claim() : pH="); s.print((uint32_t)pH,HEX); s.print(", bH="); s.println(bH);
         if (name)
         {
            CFCTok0 *pCFC= (CFCTok0*)fa.claim(sizeof(CFCTok0),brkbf);
            if  (pCFC)
            {
               brkbf= 0x0;
               pCFC->ht= 0xEA;            // String (ascii) token
               pCFC->xx= lentil(name);
               fa.append( (uint8_t*)name, pCFC->xx);
            }
         }
         if (pC)
         {
            CFCTok0 *pCFC= (CFCTok0*)fa.claim(sizeof(CFCTok0)+pC->bytesBCD4(),brkbf);
            if (pCFC)
            {
               brkbf= 0x0;
               pCFC->ht= 0xEB;            // BCD (string) token
               pCFC->xx= pC->getBCD4((uint8_t*)(pCFC+1));
            }
         }
         uint8_t nab= fa.sumFragBytesFI(1);
         if (0 == brkbf)
         { // generate CRC8 for attributes
            CFCTok0 *pCFC= (CFCTok0*)fa.claim(sizeof(CFCTok0)+1);
            pCFC->ht= 0xEC;
            if ((nab & 0x3F) == nab) { pCFC->xx= nab; } else { pCFC->xx= 0x00; }
            pCFC->a[2]= fa.crc8FromIdx(1);
            nab+= 3;
         }
         return(nab + genObjFragHdr(pH, id, 0, nab, usx));
      }
      return(0);
   } // create

}; // MFDAsm

class MFDHack : public MFDAsm
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

   void test (Stream& s, CW25QUtil& d, CClock& c)
   {
      FragAsmDbg fa;
      int r, n= MFDAsm::create(fa, "test.dat", &c, 1, 230);
      uint8_t *pH= &(fa[0]), flattened[64];

      //uint8_t n= fa.sumFragBytesFI();
      s.print("MFD hdrs ["); s.print(n); s.println("]:");

      r= fa.dump(flattened, sizeof(flattened));
      dumpHexTab(s, flattened, r);
      if (r == n)
      {
         CRC8 crc8;

         uint8_t i= n-2, r= crc8.compute( flattened+HDR_OJDF_BYTES, flattened[i]+2 );
         s.print("CRC8: ["); s.print(i); s.print("] -> "); s.println(r,HEX);
      }
      fa.test(s,0);

      int lv[2], nH= fa.bytes();
      s.print("validate:");
      lv[0]= dissect(pH,nH,s);
      //logVI(s);
      if (lv[0] > 0)
      {
         pH+= lv[0];
         nH-= lv[0];
         lv[1]= dissect(pH,nH,s);
      }
      s.println();
   } // test

}; // class MFDHack

#endif // MFD_HACKS_HPP
