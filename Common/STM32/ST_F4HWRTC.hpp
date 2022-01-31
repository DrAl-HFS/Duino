// Duino/Common/STM32/ST_F4HWRTC.hpp - STM32F4 builtin bcd-encoded RTC setup
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Jan-Feb 2022

#ifndef ST_F4HWRTC_HPP
#define ST_F4HWRTC_HPP

#include <libmaple/rtc.h>
#include <libmaple/bkp.h>

#define F4HWRTC_EPOCH_BASELINE (946684800)   // 2000/1/1 (Sat) 00:00:00 = earliest hardware representable time
#define F4HWRTC_DOW_MASK (0x7<<13) // day of week (3b 1..7)

class F4HWRTC
{
protected:
   void setRawTime (uint32_t t)
   {
      if ((t & (0x1<<24)) || (((t >> 8) & 0x7FFF) >= 0x1200)) { t|= (0x1<<22); } // set pm flag
      RTC->TR= t & 0x7F7F7F; // mask off reserved bits
   } // setRawTime

   void setRawDate (uint32_t d)
   {
      uint8_t dow= (d >> 24) & 0x7;
      dow+= (0 == dow); // valid range 1..7
      d|= dow << 13; // shift DOW into place
      RTC->DR= d & 0xFFFF3F; // mask off reserved bits
   } // setRawDate

   bool init (void)
   {
      bkp_init();             // Enable PWR clock.
      bkp_enable_writes();    // PWR_CR:DBP control bit

      // Ensure LSE mode (external 32.768kHz source)
      if (RCC_BDCR_RTCSEL_LSE != (RCC->BDCR & RCC_BDCR_RTCSEL_MASK))
      {
         RCC->BDCR= RCC_BDCR_BDRST; // Full reset of backup domain
         RCC->BDCR= (RCC_BDCR_RTCEN | RCC_BDCR_RTCSEL_LSE | RCC_BDCR_LSEON);

         int32_t c= 8<<20; // avoid hang <0.5s
         while ((0 == (RCC->BDCR & RCC_BDCR_LSERDY)) && (c > 0)) { --c; }
         //if (c <= 0) { error(); }
      }
      rtc_enter_config_mode();
      RTC->PRER = (uint32_t)((0x7F << 16) + 0xFF);
      if ((getRawDate() & 0xFFFFFF) < 0x220000)
      {
         setRawDate(0x220131); // (Mon) 22/1/31
         setRawTime(0x185948); // (pm) 18:59:48
      }
      //RCC->CR |= RTC_CR_BYPSHAD;
      *bb_perip(&RTC->CR, RTC_CR_BYPSHAD_BIT)= 1; // enable shadow bypass (reduce update latency)
      rtc_exit_config_mode();
      
      bkp_disable_writes(); // leave it as you found it
      return(true);
   } // init

public:
   F4HWRTC (void) { ; }

   uint32_t getRawTime (void)
   {
      uint32_t t= RTC->TR & 0x7F7F7F; // mask off reserved bits
      if (t & (0x1<<22)) { t^= (0x14<<20); } // relocate pm flag to top byte
      return(t);
   } // getRawTime

   uint32_t getRawDate (void)
   {
      uint32_t d= RTC->DR & 0xFFFF3F; // mask off reserved bits
      d|= (d & F4HWRTC_DOW_MASK) << 11; // shift DOW into top byte and
      d&= ~F4HWRTC_DOW_MASK;           // remove from original location
      return(d);
   } // getRawDate

}; // F4HWRTC

class RTCDebug : public F4HWRTC
{
public:
   RTCDebug (void) { ; }

   //using F4HWRTC::init;
   bool init (Stream& s) { return F4HWRTC::init(); } //s.print("F4HWRTC:CR="); s.println(RTC->CR,HEX); return(r); }
   
   void dump (Stream& s)
   {
     UU32 dt;
     int8_t i;
     char fs[4]="00/";
     dt.u32= getRawDate();
     //s.println(dt.u32,HEX);
     i= 3;
     while (i-- > 1)
     {
       hexChFromU8(fs, dt.u8[i]); 
       s.print(fs);
     }
     fs[2]= ' ';
     hexChFromU8(fs, dt.u8[0]);
     s.print(fs);
     i= dt.u8[3] & 0x7;
     if (i > 0)
     {
static const char *dow[]={"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
        s.print("("); s.print(dow[i-1]); s.print(") ");
     }
     fs[2]= ':';
     dt.u32= getRawTime();
     //s.println(dt.u32,HEX);
     if ((dt.u8[3] & 0x1) && (dt.u8[2] < 0x12)) { dt.u8[3]= bcd8Add(dt.u8+2, dt.u8[2], 0x12); } // pm -> 24hr
     i= 3;
     while (i-- > 1)
     {
       hexChFromU8(fs, dt.u8[i]); 
       s.print(fs);
     }
     hexChFromU8(fs, dt.u8[0]);
     fs[2]= 0x0; // '\n';
     s.println(fs);
   } // dumpRTC

}; // RTCDebug

#endif // ST_F4HWRTC_HPP

