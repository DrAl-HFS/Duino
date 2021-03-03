// Duino/STM32/Common/STM/ST_Timing.hpp - STM32/libMaple timer utils
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Mar 2021

#ifndef ST_TIMING_HPP
#define ST_TIMING_HPP

// Direct HW access to clock info using "Reset & Clock Control" register bank
#include <libmaple/rcc.h>

static uint32_t getClkCfg1 (void) { return( RCC_BASE->CFGR ); } // 0x51840A
//static void getClkCfg2 (uint32_t c[2]) { c[0]= RCC_BASE->CFGR; c[1]= RCC_BASE->CFGR2; } // not F103

uint32_t hwClkRate (void)
{
   uint32_t c= getClkCfg1();
   if (0xA == (c & 0xF))
   {  // PLL
      uint8_t m2=0, pllmul= (c >> 18) & 0xF;
      
      if ( ( pllmul >= 0b0010 ) && ( pllmul <= 0b0111) ) { m2= 2*(pllmul+2); }
      else if (0b1101 == pllmul) { m2= 13; }
      // Assume 8MHz input to PLL1
      //uint8_t pd= 1 + (c[1] & 0xF); // PLL1 prediv
      //uint8_t pd= 1 + ((c >> 17) & 0x1); 
      //return((m2 * 4) / pd);
      // PLL1 prediv is 1bit on F103
      if ((c >> 17) & 0x1) { return(m2 * 2); } else { return(m2 * 4); }
   }
   return(0);
} // hwClkRate

// Extend LibMaple utils, http://docs.leaflabs.com/docs.leaflabs.com/index.html
class CTimer : protected HardwareTimer
{
protected:
   volatile uint16_t nIvl; // event count
   uint16_t nRet;           // & tracking
   
public:
   CTimer (int8_t iTN=4) : HardwareTimer(iTN) { ; }

   void start (voidFuncPtr func, uint32_t ivl_us=1000)
   {
      setMode( TIMER_CH1, TIMER_OUTPUT_COMPARE );
      attachInterrupt( TIMER_CH1, func );
      setPeriod(ivl_us);            // set appropriate prescale & overflow
      setCompare( TIMER_CH1, -1 );  // clamp to overflow defined by setPeriod()
      
      refresh();  // Commit parameters count, prescale, and overflow
      resume();   // Start counting
   }
   
   void nextIvl (void) { ++nIvl; }

   uint16_t diff (void) const
   {
      int16_t d= nIvl - nRet;
      return( d>=0 ? d : (nIvl + 0xFFFF-nRet) );
   } // diff

   void retire (uint16_t d) { if (-1 == d) { nRet= nIvl; } else { nRet+= d; } }

   uint16_t tickVal (void) const { return(nIvl); }
   // DEBUG
   uint32_t hwTickRes (void) { return getPrescaleFactor(); }
   uint32_t hwTickVal (void) { return getCount(); }
   void dbgPrint (Stream& s)
   {
      s.print("hwClkCfg "); s.println(getClkCfg1(),HEX); //s.print(','); s.println(c[1],HEX);
      s.print("hwClkRate "); s.println(hwClkRate());
      s.print("hwTickRes "); s.println(hwTickRes());
      s.print("hwTickVal "); s.print(hwTickVal()); s.print(','); s.println(hwTickVal());
   } // dbgPrint

}; // CTimer

#if 0 // DEPRECATED: direct low level hacking not worth the effort...

#include <libmaple/timer.h>
#include <libmaple/bitband.h>

class CSTM32_HW_RCC //  ?!?
{
public:
   CSTM32_HW_RCC (void) { ; }

   //void enableT4 (rcc_reg_map *pRCC= RCC_BASE) { pRCC->APB1ENR|= RCC_APB1ENR_TIM4EN; }
   uint8_t checkTimers (void)
   {
      //for (int8_t i=RCC_APB1ENR_TIM2EN_BIT; i<RCC_APB1ENR_TIM7EN_BIT; i++) { rm|= bb_peri_get_bit( &(RCC_BASE->APB1ENR), i); rm<<= 1; }
      return(RCC_BASE->APB1ENR & 0xFF);
   }
   //int *p= bb_perip(RCC_BASE, RCC_APB1ENR_TIM4EN); *p= 1;
   void enableT4 (void) { bb_peri_set_bit( &(RCC_BASE->APB1ENR), RCC_APB1ENR_TIM4EN_BIT, 1 ); }
}; // CSTM32_HW_RCC

// template <int TN> ???
// .arduino15/packages/stm32duino/hardware/STM32F1/2020.4.18/system/libmaple/stm32f1/include/series/timer.h
/* STM32F1 general purpose timer register map type
typedef struct timer_gen_reg_map {
    __IO uint32 CR1;            // Control register 1 **
    __IO uint32 CR2;            // Control register 2 **
    __IO uint32 SMCR;           // Slave mode control register **
    __IO uint32 DIER;           // DMA/Interrupt enable register **
    __IO uint32 SR;             // Status register **
    __IO uint32 EGR;            // Event generation register  **
    __IO uint32 CCMR1;          // Capture/compare mode register 1 **
    __IO uint32 CCMR2;          // Capture/compare mode register 2 **
    __IO uint32 CCER;           // Capture/compare enable register **
    __IO uint32 CNT;            // Counter **
    __IO uint32 PSC;            // Prescaler **
    __IO uint32 ARR;            // Auto-reload register **
    const uint32 RESERVED1;     // Reserved **
    __IO uint32 CCR1;           // Capture/compare register 1 **
    __IO uint32 CCR2;           // Capture/compare register 2 **
    __IO uint32 CCR3;           // Capture/compare register 3 **
    __IO uint32 CCR4;           // Capture/compare register 4 **
    const uint32 RESERVED2;     // Reserved **
    __IO uint32 DCR;            // DMA control register **
    __IO uint32 DMAR;           // DMA address for full transfer **
} timer_gen_reg_map;
*/
class Timer : CSTM32_HW_RCC
{
public:
   Timer (void) { ; }

   void start (timer_gen_reg_map *pR=TIMER4_BASE)
   {
      uint8_t v= bb_peri_get_bit( &(RCC_BASE->APB1ENR), RCC_APB1ENR_TIM4EN_BIT);
      DEBUG.print("Timer4:"); DEBUG.println(checkTimers(),HEX);
      DEBUG.print("SR="); DEBUG.println(pR->SR,HEX);
      DEBUG.print("EGR="); DEBUG.println(pR->EGR,HEX);
      //bb_peri_set_bit( &(pR->EGR), 0x01, 1 ); // CC1G
      DEBUG.print("CCMR1="); DEBUG.println(pR->CCMR1,HEX);
      DEBUG.print("CCMR2="); DEBUG.println(pR->CCMR2,HEX);
      DEBUG.print("CCER="); DEBUG.println(pR->CCER,HEX);
      DEBUG.print("PSC="); DEBUG.println(pR->PSC,HEX);
      DEBUG.print("ARR="); DEBUG.println(pR->ARR,HEX);
      DEBUG.print("CCR1="); DEBUG.println(pR->CCR1,HEX);
      //pR->PSC= 0x0000; // TICK= CLK/(1+PSC)
      if (0 == v) { enableT4(); }
   }
   uint16_t poll (timer_gen_reg_map *pR=TIMER4_BASE) { return(pR->CNT); }
  
}; // Timer

#endif


#endif // ST_TIMING_HPP
