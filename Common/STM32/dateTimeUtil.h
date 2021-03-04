// Temp hack for testing

#ifndef DTU_H
#define DTU_H

// TODO: factor common code!

// Return ASCII for low 4 bits (nybble)
char hexCharL4 (uint_fast8_t u)
{
   u&= 0xF;
   if (u <= 9) { return(u+'0'); } else { return(u-9+'a'); }
} // hexCharL4

int8_t hex2ChU8 (char c[2], uint8_t u) // hexCharFromU8
{
   c[1]= hexCharL4(u);
   c[0]= hexCharL4(u>>4);
   return(2);
} // hex2ChU8

// max 2 ACII chars -> 2 digit packed bcd.
// CAVEAT: No range checking!
uint8_t bcd4FromA (const char a[], int8_t nD)
{
   uint8_t r= 0;
   if (nD > 0)
   {
      r= 0xF & (a[0] - '0');
      if (nD > 1)
      {
         r= (r << 4) | (0xF & (a[1] - '0'));
      }
   }
   return(r);
} // bcd4FromA

// CAVEAT: Ignores any leading non-digit
uint8_t bcd4FromASafe (const char a[], int8_t nA)
{
   if (nA > 0)
   {
      uint8_t mD= isdigit(a[0]);
      if (nA > 1) { mD|= isdigit(a[1])<<1; }
      switch (mD)
      {
         case 0b01 : return bcd4FromA(a+0, 1);
         case 0b10 : return bcd4FromA(a+1, 1);
         case 0b11 : return bcd4FromA(a+0, 2);
      }
   }
   return(0);
} // bcd4FromASafe

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
   if (i < sizeof(m1))
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
      if (i < sizeof(m9)) { return(9+i); }
   }
   return(-1);
} // monthNumJulian

#endif // DTU_H
