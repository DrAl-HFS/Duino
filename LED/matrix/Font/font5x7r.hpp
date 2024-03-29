// Duino/LED/matrix/Font/font5x7.h - Adapted from
//   "examples/MarqueeText" (author Marko Oette)
// distributed as part of the library package
//   "LEDMatrixDriver" (author Bartosz Bielawski).
// Split from example main, space "glyph" deleted
// index compacted, C++ helper functions added.
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Dec 2021 - Jan 2022

#ifndef FONT_5X7R_HPP
#define FONT_5X7R_HPP

// Glyph bitmaps derived from "5by7" (Bold) freely licenced (c) Peter Weigel.
// First byte specifies width, remainder give the bitmap columns, lsb uppermost.
static const uint8_t font5by7Glyph[] PROGMEM=
{ // ASCII
// 0x21
  2,0b01101111,0b01101111,                                  /* 033 = ! */
  3,0b00000011,0b00000000,0b00000011,                       /* 034 = " */
  5,0b00010100,0b01111111,0b00010100,0b01111111,0b00010100, /* 035 = # */
  5,0b00100100,0b00101010,0b01111111,0b00101010,0b00010010, /* 036 = $ */
  5,0b00100011,0b00010011,0b00001000,0b01100100,0b01100010, /* 037 = % */
  5,0b00110100,0b01001010,0b01001010,0b00110100,0b01010000, /* 038 = & */
  1,0b00000011,                                             /* 039 = ' */
// 0x28
  3,0b00011100,0b00100010,0b01000001,                       /* 040 = ( */
  3,0b01000001,0b00100010,0b00011100,                       /* 041 = ) */
  3,0b00010100,0b00001000,0b00010100,                       /* 042 = * */
  5,0b00001000,0b00001000,0b00111110,0b00001000,0b00001000, /* 043 = + */
  2,0b11100000,0b01100000,                                  /* 044 = , */
  5,0b00001000,0b00001000,0b00001000,0b00001000,0b00001000, /* 045 = - */
  2,0b01100000,0b01100000,                                  /* 046 = . */
  5,0b01000000,0b00110000,0b00001000,0b00000110,0b00000001, /* 047 = / */
// 0x40
  5,0b00111110,0b01010001,0b01001001,0b01000101,0b00111110, /* 048 = 0 */
  3,0b01000010,0b01111111,0b01000000,                       /* 049 = 1 */
  5,0b01000010,0b01100001,0b01010001,0b01001001,0b01000110, /* 050 = 2 */
  5,0b00100001,0b01000001,0b01000101,0b01001011,0b00110001, /* 051 = 3 */
  5,0b00011000,0b00010100,0b00010010,0b01111111,0b00010000, /* 052 = 4 */
  5,0b00100111,0b01000101,0b01000101,0b01000101,0b00111001, /* 053 = 5 */
  5,0b00111100,0b01001010,0b01001001,0b01001001,0b00110000, /* 054 = 6 */
  5,0b00000001,0b00000001,0b01111001,0b00000101,0b00000011, /* 055 = 7 */

  5,0b00110110,0b01001001,0b01001001,0b01001001,0b00110110, /* 056 = 8 */
  5,0b00000110,0b01001001,0b01001001,0b00101001,0b00011110, /* 057 = 9 */
  2,0b00110110,0b00110110,                                  /* 058 = : */
  2,0b01110110,0b00110110,                                  /* 059 = ; */
  4,0b00001000,0b00010100,0b00100010,0b01000001,            /* 060 = < */
  5,0b00010100,0b00010100,0b00010100,0b00010100,0b00010100, /* 061 = = */
  4,0b01000001,0b00100010,0b00010100,0b00001000,            /* 062 = > */
  5,0b00000010,0b00000001,0b01010001,0b00001001,0b00000110, /* 063 = ? */

  5,0b00111110,0b01000001,0b01011101,0b01010101,0b01011110, /* 064 = @ */
  5,0b01111110,0b00001001,0b00001001,0b00001001,0b01111110, /* 065 = A */
  5,0b01111111,0b01001001,0b01001001,0b01001001,0b00110110, /* 066 = B */
  5,0b00111110,0b01000001,0b01000001,0b01000001,0b00100010, /* 067 = C */
  5,0b01111111,0b01000001,0b01000001,0b01000001,0b00111110, /* 068 = D */
  5,0b01111111,0b01001001,0b01001001,0b01001001,0b01000001, /* 069 = E */
  5,0b01111111,0b00001001,0b00001001,0b00001001,0b00000001, /* 070 = F */
  5,0b00111110,0b01000001,0b01001001,0b01001001,0b01111010, /* 071 = G */

  5,0b01111111,0b00001000,0b00001000,0b00001000,0b01111111, /* 072 = H */
  5,0b01000001,0b01000001,0b01111111,0b01000001,0b01000001, /* 073 = I */
  5,0b00100000,0b01000001,0b01000001,0b00111111,0b00000001, /* 074 = J */
  5,0b01111111,0b00001000,0b00010100,0b00100010,0b01000001, /* 075 = K */
  5,0b01111111,0b01000000,0b01000000,0b01000000,0b01000000, /* 076 = L */
  5,0b01111111,0b00000010,0b00000100,0b00000010,0b01111111, /* 077 = M */
  5,0b01111111,0b00000110,0b00001000,0b00110000,0b01111111, /* 078 = N */
  5,0b00111110,0b01000001,0b01000001,0b01000001,0b00111110, /* 079 = O */

  5,0b01111111,0b00010001,0b00010001,0b00010001,0b00001110, /* 080 = P */
  5,0b00111110,0b01000001,0b01010001,0b00100001,0b01011110, /* 081 = Q */
  5,0b01111111,0b00001001,0b00011001,0b00101001,0b01000110, /* 082 = R */
  5,0b00100110,0b01001001,0b01001001,0b01001001,0b00110010, /* 083 = S */
  5,0b00000001,0b00000001,0b01111111,0b00000001,0b00000001, /* 084 = T */
  5,0b00111111,0b01000000,0b01000000,0b01000000,0b00111111, /* 085 = U */
  5,0b00011111,0b00100000,0b01000000,0b00100000,0b00011111, /* 086 = V */
  5,0b00111111,0b01000000,0b00110000,0b01000000,0b00111111, /* 087 = W */

  5,0b01100011,0b00010100,0b00001000,0b00010100,0b01100011, /* 088 = X */
  5,0b00000111,0b00001000,0b01110000,0b00001000,0b00000111, /* 089 = Y */
  5,0b01100001,0b01010001,0b01001001,0b01000101,0b01000011, /* 090 = Z */
  3,0b01111111,0b01000001,0b01000001,                       /* 091 = [ */
  5,0b00000001,0b00000110,0b00001000,0b00110000,0b01000000, /* 092 = \ */
  3,0b01000001,0b01000001,0b01111111,                       /* 093 = ] */
  5,0b00000100,0b00000010,0b00000001,0b00000010,0b00000100, /* 094 = ^ */
  5,0b01000000,0b01000000,0b01000000,0b01000000,0b01000000, /* 095 = _ */

  1,0b00000011,                                             /* 096 = ' */
  5,0b00100000,0b01010100,0b01010100,0b01010100,0b01111000, /* 097 = a */
  5,0b01111111,0b00101000,0b01000100,0b01000100,0b00111000, /* 098 = b */
  5,0b00111000,0b01000100,0b01000100,0b01000100,0b00101000, /* 099 = c */
  5,0b00111000,0b01000100,0b01000100,0b00101000,0b01111111, /* 100 = d */
  5,0b00111000,0b01010100,0b01010100,0b01010100,0b00011000, /* 101 = e */
  5,0b00000100,0b01111110,0b00000101,0b00000001,0b00000010, /* 102 = f */
  5,0b00011000,0b10100100,0b10100100,0b10100100,0b01111100, /* 103 = g */

  5,0b01111111,0b00000100,0b00000100,0b00000100,0b01111000, /* 104 = h */
  3,0b01000100,0b01111101,0b01000000,                       /* 105 = i */
  4,0b01000000,0b10000000,0b10000100,0b01111101,            /* 106 = j */
  5,0b01111111,0b00010000,0b00010000,0b00101000,0b01000100, /* 107 = k */
  3,0b01000001,0b01111111,0b01000000,                       /* 108 = l */
  5,0b01111100,0b00000100,0b01111100,0b00000100,0b01111000, /* 109 = m */
  5,0b01111100,0b00001000,0b00000100,0b00000100,0b01111000, /* 110 = n */
  5,0b00111000,0b01000100,0b01000100,0b01000100,0b00111000, /* 111 = o */

  5,0b11111100,0b00100100,0b00100100,0b00100100,0b00011000, /* 112 = p */
  5,0b00011000,0b00100100,0b00100100,0b00100100,0b11111100, /* 113 = q */
  5,0b01111100,0b00001000,0b00000100,0b00000100,0b00001000, /* 114 = r */
  5,0b01001000,0b01010100,0b01010100,0b01010100,0b00100000, /* 115 = s */
  5,0b00000100,0b00111110,0b01000100,0b01000000,0b00100000, /* 116 = t */
  5,0b00111100,0b01000000,0b01000000,0b00100000,0b01111100, /* 117 = u */
  5,0b00011100,0b00100000,0b01000000,0b00100000,0b00011100, /* 118 = v */
  5,0b00111100,0b01000000,0b00110000,0b01000000,0b00111100, /* 119 = w */

  5,0b01000100,0b00101000,0b00010000,0b00101000,0b01000100, /* 120 = x */
  5,0b00000100,0b01001000,0b00110000,0b00001000,0b00000100, /* 121 = y */
  5,0b01000100,0b01100100,0b01010100,0b01001100,0b01000100, /* 122 = z */
  3,0b00001000,0b00110110,0b01000001,                       /* 123 = { */
  1,0b01111111,                                             /* 124 = | */
  3,0b01000001,0b00110110,0b00001000,                       /* 125 = } */
  5,0b00011000,0b00000100,0b00001000,0b00010000,0b00001100  /* 126 = ~ */
}; // font5by7Glyph

// Hacky index truncation macro
#define FIM(x) (0xFF&(x-3))
// index into map for faster lookup; indices truncated for storage & lookup efficiency
// (corrected in lookup routine)
static const uint8_t font5by7Index[] PROGMEM=
{ // ' ' deleted
            FIM(3),  FIM(6),  FIM(10), FIM(16), FIM(22), FIM(28), FIM(34),  // ! '
   FIM(36), FIM(40), FIM(44), FIM(48), FIM(54), FIM(57), FIM(63), FIM(66),  // ( /
   FIM(72), FIM(78), FIM(82), FIM(88), FIM(94), FIM(100),FIM(106),FIM(112), // 0 7
   FIM(118),FIM(124),FIM(130),FIM(133),FIM(136),FIM(141),FIM(147),FIM(152), // 8 ?
   FIM(158),FIM(164),FIM(170),FIM(176),FIM(182),FIM(188),FIM(194),FIM(200), // @ G
   FIM(206),FIM(212),FIM(218),FIM(224),FIM(230),FIM(236),FIM(242),FIM(248), // H O
   FIM(254),  // P
   // index truncation from 'Q' - '!'
            FIM(260),FIM(266),FIM(272),FIM(278),FIM(284),FIM(290),FIM(296), // Q W
   FIM(302),FIM(308),FIM(314),FIM(320),FIM(324),FIM(330),FIM(334),FIM(340),
   FIM(346),FIM(348),FIM(354),FIM(360),FIM(366),FIM(372),FIM(378),FIM(384),
   FIM(390),FIM(396),FIM(400),FIM(405),FIM(411),FIM(415),FIM(421),FIM(427),
   FIM(433),FIM(439),FIM(445),FIM(451),FIM(457),FIM(463),FIM(469),FIM(475),
   FIM(481),FIM(487),FIM(493),FIM(499),FIM(503),FIM(505),FIM(509)
};
#define FIM_TRUNC_START ('Q' - '!')

static const uint8_t extGlyph[] PROGMEM=
{
  3,0b00000010,0b00000101,0b00000010  // degree symbol
}; // extGlyph

bool isGlyphIndex (const uint8_t iG) { return(iG < sizeof(font5by7Index)); }
bool nonGlyphChar (const char ch) { return !isGlyphIndex(ch-'!'); }

int16_t glyphIndexRaw (const uint8_t iG)
{
  if (isGlyphIndex(iG))
  {
    int16_t i= pgm_read_byte( font5by7Index + iG );
    if (iG >= FIM_TRUNC_START) { i |= 0x100; }
    return(i);
  } // else
  return(-1);
} // glyphIndexRaw

int16_t glyphIndexASCII (const char ch) { return glyphIndexRaw(ch - '!'); }

// CAVEATS: assumes valid index, supports <=16bit address space only
int8_t glyphWidth (const int16_t iG, const uint16_t base=font5by7Glyph) { return pgm_read_byte( base + iG ); }
uint8_t glyphCol (const int16_t iG, const int8_t iC, const uint16_t base=font5by7Glyph) { return pgm_read_byte( base + iG + 1 + iC ); }
int8_t glyphCopy (uint8_t c[6], const int16_t iG, const uint16_t base=font5by7Glyph)
{
   int8_t w= glyphWidth(iG,base);
   for (int8_t i=0; i<w; i++) { c[i]= glyphCol(iG,i,base); }
   return(w);
} // glyphCopy

#ifndef DUMP
#define DUMP //DUMP
#else

//#define DUMP(s) verifyFontIndex(s)

void verifyFontIndex (Stream& s)
{
   int16_t j=0;
   for (int8_t iC=0; iC<sizeof(font5by7Index); iC++)
   {
      int16_t i= glyphIndexRaw(iC);
      uint8_t w= glyphWidth(i);
      s.print(iC); s.print(':');
      s.print(i); s.print(','); s.print(w); s.print(';');
      w= glyphWidth(j);
      s.print(j); s.print(','); s.println(w);
      j+= 1+w;
   }
} // verifyFontIndex

void dumpRawIndex (Stream& s)
{
   int16_t j=0;
   for (int8_t iC=0; iC<sizeof(font5by7Index); iC++)
   {
      s.print("FIM("); s.print(j); s.print("), ");
      j+= 1+glyphWidth(j);
   }
} // dumpRawIndex

#endif // DUMP

#endif // FONT_5X7R_HPP


