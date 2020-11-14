// Duino/Common/numCh.hpp - Arduino-AVR hardware counter hacks (broken)
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Oct-Nov 2020

#ifndef DA_COUNTING_H
#define DA_COUNTING_H

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
  uint16_t o, c;
  
  // something missing: counts when T1 grounded...
  CCountExtBase ()
  {
    pinMode(PIN_T1, INPUT); // ???
    //c= 0; // unnecessary
#ifdef AVR
    //ASSR= (1 << EXCLK) | ;
    TCNT1=  0;   // preload timer
    TCCR1A= 0; // 1 << WGM12 // CTC
    TCCR1B= (1 << CS12) | (1 << CS11) | (1 << CS10);  // external "clock" source, rising edge on T1
    OCR1A=  1000;
    TIMSK1= (1 << TOIE1); // | (1 << OCIE1A);  // Overflow Interrupt Enable/Output compare
#endif
  } // CExtClockCountBase
  
  
}; // CCountExtBase 


#endif //  DA_COUNTING_H

