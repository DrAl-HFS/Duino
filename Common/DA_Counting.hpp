// Duino/Common/numCh.hpp - Arduino-AVR hardware counter hacks (broken)
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Oct-Nov 2020

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

// Processor specific variants

#ifdef AVR
#define PIN_T0 4
#define PIN_T1 5
#endif

#ifdef SAM  // SamD21/51
#endif

/***/

#if 1

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
   
   uint16_t getCountU16 (void) { return(TCNT1); }
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


#else

class CCountExtBase
{
protected:
   volatile TimerIvl stamp; // NB: type visibility due to order of header inclusion in top level (.ino) module
public:
   volatile uint8_t nIvl;  // ISR mini-counter
   uint8_t last, nRet; // Previous time stamp & ISR mini-counter (since last "upstream" application check)

public:
   uint16_t n, t;

   CCountExtBase (void) {;}

   // deferred start essential to prevent overwrite by Arduino setup
   // (assume memclear, static construction, then handoff to "app" level)
   void start (uint16_t ivl=1000)
   {
      pinMode(PIN_T1, INPUT); // ???
      //ASSR= (1 << EXCLK) | ;
      TCNT1=  0;   // preload timer
      TCCR1A= 0; //
      TCCR1B= (1 << WGM12) | (1 << CS12) | (1 << CS11) | (1 << CS10);  // CTC & external "clock" source, rising edge on T1
      OCR1A=  ivl-1;
      TIMSK1= (1 << OCIE1A);  // Output Compare A Interrupt Enable (1 << TOIE1) Overflow Interrupt Enable
   } // start

   void event (void) { stamp= TCNT2; ++nIvl; } // ISR

   uint8_t accumRate (uint8_t tm)
   {
      int8_t d;
      //n= OCR1A + 1;
      //t= 4; // Hack! us / tick
      //d= stamp - last;
      //if (d > 0) { t+= (uint8_t)d; } else { t+=  (uint8_t)(0xFF + d); }
      t+= tm;
      // NB: does not account for compare threshold change
      d= nIvl - nRet;
      if (d > 0) { n+= (uint8_t)d; } else { n+=  (uint8_t)(0xFF + d); }
      nRet= nIvl;
      last= stamp;
      return(d);
   } // accumRate

   uint16_t measure (void) // TODO : rate per micro/milli/sec
   {
      //uint16_t r= (n * 1000) / (t*4); // hacky assumption of externally defined tick rate...
      uint16_t r= n;
      if (t > 1) { r= n / t; }
      return(r);
   } // measure

   void reset (void) { n= 0; t= 0; }
}; // CCountExtBase

#endif

#endif //  DA_COUNTING_HPP

