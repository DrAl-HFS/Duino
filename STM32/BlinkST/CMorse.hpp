// Duino/STM32/BlinkST/CMorse.hpp - International Morse Code pattern processing
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Feb 2021

#ifndef CMORSE_HPP
#define CMORSE_HPP

#include "morsePattern.h"

// Pattern Send State
class CMorsePSS 
{
protected:
  uint8_t c;
  int8_t n, i;

  void set (const uint8_t codeBits, const int8_t nBits) { c= codeBits; n= nBits; i= n; }
  
public:
  CMorsePSS (void) { ; }

  bool setSafe (const uint8_t codeBits, const int8_t nBits)
  { 
    if ((nBits < 0) || (nBits > 8)) { return(false); } //else
    set(codeBits, nBits);
    return(true);
  } // set 
  
  bool setM5 (const uint8_t m5=0) { set(m5 & IMC5_C_MASK, m5 >> IMC5_N_SHIFT); }
  
  int8_t next (void)
  {
    int8_t v= 0x2; // end of pattern
    if (i >= 0) { v= (c >> --i) & 0x1; }
    return(v);
  }
}; // CMorsePSS

int8_t classifyASCII (const char a)
{
  if (a >= '0')
  {
    if (a <= '9') { return('0'); } 
    if (a >= 'A')
    {
      if (a <= 'Z') { return('A'); }
      if ((a >= 'a') && (a <= 'z')) { return('a'); }
    }
  }
  return(-1);
} // classifyASCII

#if 1

// String Send State
class CMorseSSS : CMorsePSS
{
protected:
  const char *s;
  int8_t iS;  // no long strings!

  bool setASCII (const char a)
  {
    switch (classifyASCII(a))
    {
      case '0' :  return CMorsePSS::setM5(gNumIMC5[a-'0']); // break;
      case 'a' :  return CMorsePSS::setM5(gAlphaIMC5[a-'a']); // break;
      case 'A' :  return CMorsePSS::setM5(gAlphaIMC5[a-'A']); // break;
    }
    return(false);
  } // setASCII
  
  bool nextASCII (void)
  {
    char a;
    if (NULL == s) { return(false); }
    do
    {
      a= s[++iS];
      if (0 == a) { return(false); }
      //Serial.print("nextASCII() - "); Serial.print(iS); Serial.print("->"); Serial.println(a);
    } while (!setASCII(a)); // skip any non-translateable
    return(true);
  } // nextASCII
  
public:
  CMorseSSS (void) { ; }
  
  void set (const char *str)
  { 
    s= str;
    reset();
  } // set
  
  void reset (void)
  {
    iS= 0;
    if (NULL == s) { setASCII(0); }
    else { setASCII(s[iS]); }
  } // reset
  
  int8_t next (void)
  {
    int8_t r= CMorsePSS::next();
    if (r >= 0x2)
    { 
      if (!nextASCII()) { r|= 0x4; }
    }
    return(r);
  }
}; // CMorseSSS

#else

class CMorseSSS { public: int i; CMorseSSS (void) { ; } };

#endif

#endif // CMORSE_HPP
