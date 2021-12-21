// Duino/LED/matrix/scroll.hpp - Scrolling text glyph rendering utils.
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Dec 2021

class GlyphScroll
{
protected:
   int16_t iG;       // index into glyph map (9b+sign)
   uint8_t wG, iC;   // glyph width and column index (3b ea)
  
public:
   GlyphScroll (void) { set(0x00); }

   bool set (const char ch) { iC= wG= 0; iG= glyphIndexASCII(ch); return(iG >= 0); }

   uint8_t nextCBM (void)
   {
      if (iC < wG) { return glyphCol(iG, iC++); }
      return(0x00);
   } // nextCBM
  
   uint8_t nextGlyph (const char ch)
   {
      if (set(ch)) { wG= glyphWidth(iG); }
      return(wG);
   } // nextGlyph

   // DEBUG
   void dump (Stream& s) const
   {
      s.print('['); s.print(iG); s.print(']'); s.print(wG); s.print(','); s.println(iC);
   }
}; // GlyphScroll

#ifndef MAX // C++ max() strangely broken on some platforms...
#define MAX(a,b) max(a,b)
//#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

class TextScroll : public GlyphScroll
{
   uint8_t iT, gap;
   
   uint8_t next (const char txt[])
   {
      int8_t m= 10; // avoid bad message jam
      char ch= txt[iT];
      gap= 1; // default
      do
      {
         while (nonGlyphChar(ch) && (m-- > 0))
         {
            iT++; // always skip
            switch(ch)
            {
               case 0x00 : iT= 0; break; // wrap
               case ' ' : gap+= 2; break;
               case '\t' : gap= MAX(gap,8); break;
               case '\r' :
               case '\n' : gap= MAX(gap,32); break;
            }
            //DEBUG.print('['); DEBUG.print(ch); DEBUG.print(':'); DEBUG.print(gap);
            ch= txt[iT];
         }
      } while ((0 == nextGlyph(ch)) && (m > 0));
      iT++;
      return(gap);
   } // next
   
public:
   TextScroll (void) : GlyphScroll(), iT{0}, gap{0} { ; }

   uint8_t nextCBM (const char txt[])
   {
      if (gap > 0) { --gap; return(0x00); }
      // else
      uint8_t cbm= GlyphScroll::nextCBM();
      if (0x00 == cbm)
      {
         if (0 == next(txt))
         {  // Gapless mode (possible future extension)
            cbm= GlyphScroll::nextCBM();
         }
      }
      return(cbm);
   } // nextCBM

}; // TextScroll


