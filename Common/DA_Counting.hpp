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
public:
   UU32  c[2];
   uint16_t u;
  
   // something missing: counts when T1 grounded...
   CCountExtBase (void)
   {
      pinMode(PIN_T1, INPUT); // ???
      //c= 0; // unnecessary
#ifdef AVR
      //ASSR= (1 << EXCLK) | ;
      TCNT1=  0;   // preload timer
      TCCR1A= 0; //
      TCCR1B= (1 << WGM12) | (1 << CS12) | (1 << CS11) | (1 << CS10);  // CTC & external "clock" source, rising edge on T1
      OCR1A=  0xFFFF; // 10000 - 1;
      TIMSK1= (1 << OCIE1A);  // Output Compare A Interrupt Enable (1 << TOIE1) Overflow Interrupt Enable
#endif
   } // CCountExtBase
  
   // Appears to run at 32MHz!? even when T1 is held low... something very wrong
   void update (void) { OCR1A= 0xFFFF; ++u; } // ISR
  
   uint32_t diff (void)
   {
      c[0]= c[1];
      c[1].u16[0]= TCNT1; c[1].u16[1]= u; // should disable interrupts for this..
      return(c[1].u32 - c[0].u32);
   }
}; // CCountExtBase 


#endif //  DA_COUNTING_HPP

