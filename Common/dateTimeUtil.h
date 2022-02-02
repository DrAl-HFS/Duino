// Temp hack for testing

#ifndef DTU_H
#define DTU_H

#include "DN_Util.hpp"
/* TODO: factor common code!

// Return ASCII for low 4 bits (nybble)
char hexCharL4 (uint_fast8_t u, const char a='a')
{
   u&= 0xF;
   if (u <= 9) { return(u+'0'); } else { return(u-0xA+a); }
} // hexCharL4

int8_t hex2ChU8 (char c[2], uint8_t u) // 
{
   c[1]= hexCharL4(u);
   c[0]= hexCharL4(u>>4);
   return(2);
} // hex2ChU8
*/

// Caveat: brainlessly expects hh:mm:ss
// Outputs hms (or smh) packed bcd sequence
int8_t bcd4FromTimeA (uint8_t bcd[3], const char *timeStr, uint8_t hms=2)
{
   hms&= 2; // ensure 2 or 0
   bcd[2-hms]= bcd4FromA(timeStr+0,2);
   bcd[1]=     bcd4FromA(timeStr+3,2);
   bcd[hms]=   bcd4FromA(timeStr+6,2);
   return(3);
} // bcd4FromTimeA

int8_t monthNumJulian (const char a[]);

// super hacky
int8_t bcd4FromDateA (uint8_t bcd[3], const char *dateStr, uint8_t ymd=2)
{
   int8_t i=0;
   bcd4FromU8(bcd+1, monthNumJulian(dateStr+i));
   ymd&= 2; // ensure 2 or 0
   i+= 4;
   bcd[ymd]=   bcd4FromASafe(dateStr+i,2);
   i+= 3;
   bcd[2-ymd]= bcd4FromA(dateStr+i,2);
   return(3);
} // bcd4FromDateA

int findCh (const char c, const char a[], const int n)
{
   int i= 0;
   while ((i < n) && (a[i] != c)) { ++i; }
   return(i);
} // findCh

// Minimal & hacky Julian calendar month English text discrimination
// CAVEAT : GIGO
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

#endif // DTU_H
