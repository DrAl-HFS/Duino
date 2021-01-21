// Duino/Common/DA_Conting.hpp - Arduino-AVR hardware counter hacks
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Oct-Dec 2020

#ifndef DA_COUNTING_HPP
#define DA_COUNTING_HPP

//#include <?>

//#if 0 //def __cplusplus
//extern "C" {
//#endif

//#if 0 //def __cplusplus
//} // extern "C"
//#endif

/***/

#define PIN_T0 4
#define PIN_T1 5

#ifndef SAMD  // SamD21/51
#endif

/***/

uint16_t diffWrapU16 (uint16_t a, uint16_t b)
{
   if (a >= b) { return(a-b); } else { return(a+0xFFFF-b); }
} // diffWrapU16

class CCountExtBase
{
protected:
   volatile uint8_t nOflo;  // ISR mini-counter

public:
   CCountExtBase (void) {;}

   // deferred start essential to prevent overwrite by Arduino setup
   // (assume memclear, static construction, then handoff to "app" level)
   void start (void)
   {
      pinMode(PIN_T1, INPUT); // ???
      //ASSR= (1 << EXCLK) | ;
      TCNT1=  0;  // set counter ?
      TCCR1A= 0;  //
      TCCR1B= (1 << CS12) | (1 << CS11); // | (1 << CS10);  // ext clk T1, rising / falling 
      TIMSK1= (1 << TOIE1); //  Overflow Interrupt Enable
   } // start

   void event (void) { ++nOflo; } // ISR
   
   uint16_t getCountU16 (void) { return(TCNT1); } // safe ??
}; // CCountExtBase

struct SRate { uint16_t c, t; };
#define RATE_RES_COUNT (1<<2)
#define RATE_RES_MASK (RATE_RES_COUNT-1)

class CRateEst : public CCountExtBase
{
public:
   SRate ref, res[RATE_RES_COUNT];
   uint8_t iRes;
   
   CRateEst (void) {;}
   
   using CCountExtBase::start;
   
   int8_t update (uint16_t tick)
   {
      uint16_t c= getCountU16();
      int8_t r= -1;
      res[iRes].c= diffWrapU16(c, ref.c);
      if (res[iRes].c > 0)
      {
         res[iRes].t= tickDiffWrapU16(tick, ref.t);
         r= (res[iRes].c >= 100) || (res[iRes].t >= 100);
         if (r) { iRes= (iRes+1) & RATE_RES_MASK; }
      }
      if (0 != r) { ref.c= c; ref.t= tick; }
      return(r);
   } // update

   const SRate& get (void) const { return(res[(iRes-1) & RATE_RES_MASK]); }
}; // CRateEst

#endif //  DA_COUNTING_HPP

