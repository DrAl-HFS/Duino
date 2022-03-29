// Temp hack for testing

#ifndef DTU_HPP
#define DTU_HPP

#include "DN_Util.hpp"

// Caveat: brainlessly expects hh:mm:ss
// Outputs hms (or smh) packed bcd sequence
int8_t bcd4FromTimeA (uint8_t bcd[3], const char a[], uint8_t hms=2)
{
   hms&= 2; // ensure 2 or 0
   bcd[2-hms]= bcd4FromA(a+0,2);
   bcd[1]=     bcd4FromA(a+3,2);
   bcd[hms]=   bcd4FromA(a+6,2);
   return(3);
} // bcd4FromTimeA

// As above but 8b binary values
int8_t u8FromTimeA (uint8_t u[3], const char a[], uint8_t hms=2)
{
   int8_t r= bcd4FromTimeA(u, a, hms);
   for (int8_t i=0; i<r; i++) { u[i]= fromBCD4(u[i],2); }
   return(r);
} // u8FromTimeA
/*
uint32_t secFromTimeA (const char a[])
{
   uint32_t s= 0;
   uint8_t u[3];
   int8_t r= u8FromTimeA(u,a,2);
   if (r >= 3) { s= u[2]; }
   if (r >= 2) { s+= u[1] * 60; }
   if (r >= 1) { s+= u[0] * 60 * 60; }
   return(s);
} // secFromTimeA
*/
int8_t monthNumJulian (const char a[]); // forward prototype

// super hacky
int8_t bcd4FromDateA (uint8_t bcd[3], const char *a, uint8_t ymd=2)
{
   int8_t i=0;
   bcd4FromU8(bcd+1, monthNumJulian(a+i));
   ymd&= 2; // ensure 2 or 0
   i+= 4;
   bcd[ymd]=   bcd4FromASafe(a+i,2);
   i+= 5;
   bcd[2-ymd]= bcd4FromA(a+i,2);
   return(3);
} // bcd4FromDateA

int8_t u8FromDateA (uint8_t u[3], const char a[], uint8_t ymd=2)
{
   int8_t r= bcd4FromDateA(u, a, ymd);
   for (int8_t i=0; i<r; i++) { u[i]= fromBCD4(u[i],2); }
   return(r);
} // u8FromTimeA

// CAVEATS:
// 1) Input month *number* (1..12) not index (but zero->"no month" is valid input).
// 2) leap years ignored...
int monthJulianDays (const uint8_t nMonth, const bool integral)
{
static const uint8_t md[12]={31,28,31,30,31,30,31,31,30,31,30,31};
   if (nMonth <= 12)
   {
      if (integral)
      {
         uint16_t n=0;
         for (int i=0; i<nMonth; i++) { n+= md[i]; }
         return(n);
      }//else
      if (nMonth > 0) { return(md[nMonth-1]); }
   }
   return(0);
} // monthJulianDays

// Debug Hack
unsigned int setDaysJulianU8 (unsigned int d[2], const uint8_t ymd[3])
{
   d[0]= ((unsigned int)ymd[0] * ((365 << 2) + 1)) >> 2; // * 365.25
   d[1]= monthJulianDays(ymd[1]-1,true);
   if ((ymd[1] >= 3) && (0 == (ymd[0] % 4))) { d[1]++; } // leap day (century rule ignored...)
   return(d[0] + d[1] + ymd[2]);
} // setDaysJulianU8

unsigned int sumDaysJulianU8 (const uint8_t ymd[3])
{
   unsigned int nd= ((unsigned int)ymd[0] * ((365 << 2) + 1)) >> 2;
   nd+= monthJulianDays(ymd[1]-1,true);
   if ((ymd[1] >= 3) && (0 == (ymd[0] % 4))) { nd++; } // leap day
   return(nd + ymd[2]);
} // sumDaysJulianU8

int findCh (const char c, const char a[], const int n)
{
   int i= 0;
   while ((i < n) && (a[i] != c)) { ++i; }
   return(i);
} // findCh

// consider : dayNumFromEnA
int8_t dayNumEn (const char a[])
{
static const char d6[6]={'M','T','W','T','F','S'};
   int8_t i= findCh(toupper(a[0]), d6, sizeof(d6));
   if (i < 4) { return(1+i); }
   else if (5 == i)
   {
      i+= 1+('u' == tolower(a[1]));
      return(i);
   }
   return(-1);
} // dayNumEn

// Minimal & hacky Julian calendar month English ASCII text discrimination
// CAVEAT : GIGO
// consider : monthJulianNumFromEnA
int8_t monthNumJulian (const char a[])
{
static const char m1[4]={'J','F','M','A'};
static const char m9[4]={'S','O','N','D'};
   char c= toupper(a[0]);
   int8_t i;
   i= findCh(c, m1, sizeof(m1));
   if (i < (int)sizeof(m1))
   {  // likely
      if (1 != i)
      {
         c= tolower(a[2]);
         switch(i)
         {
            case 0 :
               if ('l' == c) { return(7); } // Jul : J?l
               else if ('u' == tolower(a[1])) { return(6); } // Jun : Ju*
               break; // Jan : J*
            case 2 :
               if ('y' == c) { return(5); } // May : M?y
               break; // Mar : M*
            case 3 :
               if ('g' == c) { return(8); } // Aug : A?g
               break; // Apr : A*
         }
      }
      return(1+i); // Jan, Feb, Mar, Apr
   }
   else
   {  // First character only!
      i= findCh(c, m9, sizeof(m9));
      if (i < (int)sizeof(m9)) { return(9+i); }
   }
   return(-1);
} // monthNumJulian

#endif // DTU_HPP
