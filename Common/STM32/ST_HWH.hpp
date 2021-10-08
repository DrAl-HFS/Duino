// Duino/Common/STM32/ST_HWH.hpp - Hardware hackery factored out
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Oct 2021

#ifndef ST_HWH_HPP
#define ST_HWH_HPP

#include <libmaple/rcc.h>
#include <libmaple/timer.h>
#include <libmaple/bitband.h>

#ifndef ARDUINO_ARCH_STM32F4

// F103 hackery using some libmaple hooks

// Direct HW access to clock info using "Reset & Clock Control" register bank

static uint32_t getClkCfg1 (void) { return( RCC_BASE->CFGR ); } // 0x51840A
//static void getClkCfg2 (uint32_t c[2]) { c[0]= RCC_BASE->CFGR; c[1]= RCC_BASE->CFGR2; } // not F103

#endif

uint32_t hwClkRate (void)
{
   uint32_t c= getClkCfg1();
   if (0xA == (c & 0xF))
   {  // PLL
      uint8_t m2=0, pllmul= (c >> 18) & 0xF;
      
      if ( ( pllmul >= 0b0010 ) && ( pllmul <= 0b0111) ) { m2= 2*(pllmul+2); } // (4..9) * 2
      else if (0b1101 == pllmul) { m2= 13; } // 6.5 * 2
      // Assume 8MHz input to PLL1
      //uint8_t pd= 1 + (c[1] & 0xF); // PLL1 prediv
      //return((m2 * 4) / pd);
      // PLL1 prediv is 1bit on F103: pd= 1 + ((c >> 17) & 0x1); =1,2
      if ((c >> 17) & 0x1) { return(m2 * 2); } else { return(m2 * 4); }
   }
   return(0);
} // hwClkRate

void enableT4 (void) { bb_peri_set_bit( &(RCC_BASE->APB1ENR), RCC_APB1ENR_TIM4EN_BIT, 1 ); }


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
class TimerHack : CSTM32_HW_RCC
{
public:
   TimerHack (void) { ; }

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
  
}; // TimerHack

#endif // !ARDUINO_ARCH_STM32F4


#endif // ST_HWH_HPP
