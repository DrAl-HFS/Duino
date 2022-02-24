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

#ifndef RTC_CR_FMT_BIT
#define RTC_CR_FMT_BIT 6
#endif

// DISPLACE -> ???
// masked comparison
int mcmp (const uint32_t a, const uint32_t b, const uint32_t m=0xFFFFFF) { return sign((a&m) - (b&m)); }

bool dateValid (const UU32 dmy) // dd:mm:yy
{
   if (  (0 != dmy.u8[0]) && (0 != dmy.u8[0]) &&
         (cmpBCD4(dmy.u8[0], 0x31) <= 0) &&
         (cmpBCD4(dmy.u8[1], 0x12) <= 0) &&
         isBCD4(dmy.u8[2]) )
   {
      switch(dmy.u8[1])
      {
         case 0x02 :
         {
            uint8_t y= bcd4ToU8(dmy.u8[2],2);
            uint8_t d= 0x28 + (0 == (y % 4)); // leap year hack (imperfect Gegorian rule)
            return(cmpBCD4(dmy.u8[0], d) <= 0);
         } // break;
         case 0x04 :
         case 0x06 :
         case 0x09 :
         case 0x11 :
            return(cmpBCD4(dmy.u8[0], 0x30) <= 0); // break;
      }
   }
   return(false);
} // dateValid

bool timeValid (const UU32 smh, const uint8_t hrMax=0x24) // ss:mm:hh
{
   return( (cmpBCD4(smh.u8[0], 0x59) <= 0) &&
            (cmpBCD4(smh.u8[1], 0x59) <= 0) &&
            (cmpBCD4(smh.u8[2], hrMax) <= 0) );
} // timeValid

class F4HWRTC // CAVEAT : no error checking
{
protected:
   bool mode12h (void) const { return *bb_perip(&RTC->CR, RTC_CR_FMT_BIT); }

   void setRawTime (uint32_t t)
   {  // if (((t >> 8) & 0x7FFF) >= 0x1200)
      if (mode12h() && (t & (0x1<<24))) { t|= (0x1<<22); } // relocate pm flag
      RTC->TR= t & 0x7F7F7F; // mask off reserved bits
   } // setRawTime

   void setRawDate (uint32_t d)
   {
      uint8_t dow= (d >> 24) & 0x7;
      dow+= (0 == dow); // valid range 1..7
      d|= dow << 13; // shift DOW into place
      RTC->DR= d & 0xFFFF3F; // mask off reserved bits
   } // setRawDate

   uint8_t init (uint32_t t=0, uint32_t d=0)
   {
      uint8_t f;

      bkp_init();             // Enable PWR clock.
      bkp_enable_writes();    // PWR_CR:DBP control bit

      f= *backup(0) > 0;
      if (0 == f) { *backup(0)= 0x1; }

      // Ensure LSE mode (external 32.768kHz source)
      if (RCC_BDCR_RTCSEL_LSE != (RCC->BDCR & RCC_BDCR_RTCSEL_MASK))
      {
         RCC->BDCR= RCC_BDCR_BDRST; // Full reset of backup domain
         RCC->BDCR= (RCC_BDCR_RTCEN | RCC_BDCR_RTCSEL_LSE | RCC_BDCR_LSEON);

         int32_t c= 8<<20; // avoid hang <0.5s
         while ((0 == (RCC->BDCR & RCC_BDCR_LSERDY)) && (c > 0)) { --c; }
         //if (c <= 0) { error(); }
         f|= 0x08;
      }
      rtc_enter_config_mode();
      RTC->PRER = (uint32_t)((0x7F << 16) + 0xFF);
      //if (0 == d) { d= 0x6220101; } // (Sat) 22/1/1
      //if (0 == t) { t= 0x105923; } // 10:59:23
      if (0 == (0x1 & f))
      {  // must set clock
         if (0 != d) { setRawDate(d); f|= 0x20; }
         if (0 != t) { setRawTime(t); f|= 0x40; }
      }
      else
      {  // update?
         int dd= 0;
         f|= 0x10;
         if (0 != d)
         {
            dd= mcmp(d,getRawDate());
            if (dd > 0) { setRawDate(d); f|= 0x20; }
         }
         if ((0 != t) && (dd > 0))
         {
            dd= mcmp(t,getRawTime());
            if (0 != dd) { setRawTime(t); f|= 0x40; }
         }
      }
      //RCC->CR |= RTC_CR_BYPSHAD;
      *bb_perip(&RTC->CR, RTC_CR_FMT_BIT)= 0;      // 24h time format
      *bb_perip(&RTC->CR, RTC_CR_BYPSHAD_BIT)= 1;  // enable shadow bypass (reduce update latency)
      rtc_exit_config_mode();
      bkp_disable_writes(); // leave it as you found it
      return(f);
   } // init

   uint32_t *backup (uint8_t i)
   {
      if (i < 20)
      {  // Neither RTC_BKP0R nor BKP0R are defined
         uint32_t *p= ((uint32_t*)RTC)+(0x50>>2);
         return(p+i);
      } // else
      return(NULL);
   } // backup

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
   bool init (Stream& s, const char *timeStr=NULL, const char *dateStr=NULL)
   {
      UU32 smh={0}, dmy={0};

      //if (mode12h()) { s.println("RTCDebug::init() - 12hr fmt must be disabled..."); }
      if (timeStr)
      {
         bcd4FromTimeA(smh.u8, timeStr, 0);
         //s.print(timeStr); s.print(" -> "); s.println(smh.u32, HEX);
         if (!timeValid(smh)) { smh.u32= 0; }
      }
      if (dateStr)
      {
         bcd4FromDateA(dmy.u8, dateStr, 0);
         //s.print(dateStr); s.print(" -> "); s.println(dmy.u32, HEX);
         if (!dateValid(dmy)) { dmy.u32= 0; }
      }
      uint8_t f= F4HWRTC::init(smh.u32, dmy.u32);
      s.print("RTCDebug::init() - f=0b"); s.println(f,BIN);
      return(true);
   } // init

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
     //if (mode12h() && (dt.u8[3] & 0x1) && (dt.u8[2] < 0x12)) { dt.u8[3]= bcd4Add2(dt.u8+2, dt.u8[2], 0x12); } // pm -> 24hr
     i= 3;
     while (i-- > 1)
     {
       hexChFromU8(fs, dt.u8[i]);
       s.print(fs);
     }
     hexChFromU8(fs, dt.u8[0]);
     fs[2]= 0x0; // '\n';
     s.println(fs);
   } // dump

}; // RTCDebug

#endif // ST_F4HWRTC_HPP

