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
#ifdef AVR
      //ASSR= (1 << EXCLK) | ;
      TCNT1=  0;   // preload timer
      TCCR1A= 0; //
      TCCR1B= (1 << WGM12) | (1 << CS12) | (1 << CS11) | (1 << CS10);  // CTC & external "clock" source, rising edge on T1
      OCR1A=  ivl-1;
      TIMSK1= (1 << OCIE1A);  // Output Compare A Interrupt Enable (1 << TOIE1) Overflow Interrupt Enable
#endif
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


#endif //  DA_COUNTING_HPP

