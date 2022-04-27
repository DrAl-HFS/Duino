// Duino/Common/CCommonSPI.hpp - 'Duino (Adafruit Wire) encapsulation of common I2C functionality.
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Mar 2022

#ifndef CCOMMON_I2C_HPP
#define CCOMMON_I2C_HPP

#ifndef I2C
// ~/Tool/arduino-1.8.13/hardware/arduino/avr/libraries/Wire
#define I2C Wire
// Changing definitions seems to have no effect :(
//#define TWI_BUFFER_LENGTH 40 // used in twi.h - AVR specific ?
//#define BUFFER_LENGTH 40
#include <Wire.h>
// NB: device address for wire library is given as 7msb only
// i.e hwa= (da<<1)| RNW
#endif

class CCommonI2C
{
protected:

   int read (uint8_t b[], const int n)
   {
      int i= 0; //i= I2C.available(); if (i < n) { n= i; }
      for (; i<n; i++) { b[i]= I2C.read(); }
      return(i);
   } // write

   // should already be implemented?
   int write (const uint8_t b[], const int n)
   {
      int i= 0;
      for (; i<n; i++) { I2C.write(b[i]); }
      return(i);
   } // write

public:
   CCommonI2C (void) { ; }

   // void begin (void) { I2C.begin(); }

   int readFrom (const uint8_t devAddr, uint8_t b[], int n)
   {
      n= I2C.requestFrom(devAddr,(uint8_t)n);
      return read(b,n);
   } // readFrom

   int writeTo (const uint8_t devAddr, const uint8_t b[], int n)
   {
      I2C.beginTransmission(devAddr);
      n= write(b,n);
      int r= I2C.endTransmission();
      if (0 == r) { return(n); } //else
      return(-r);
   } // writeTo

   // convenience wrapper
   int writeTo (const uint8_t devAddr, const uint8_t b) { return writeTo(devAddr, &b, 1); }

}; // CCommonI2C

// Endian reversal extensions
class CCommonI2CX1 : public CCommonI2C
{
protected:
   int readRev (uint8_t b[], const int n)
   {
      int i= n;
      while (i-- > 0) { b[i]= I2C.read(); }
      return(n);
   } // write

   int writeRev (const uint8_t b[], const int n)
   {
      int i= n;
      while (i-- > 0) { I2C.write(b[i]); }
      return(n);
   } // write

public:
   //CCommonI2CX1 (void) { ; }

   int readFromRev (const uint8_t devAddr, uint8_t b[], int n)
   {
      if (n <= 0) { return(0); }
      n= I2C.requestFrom(devAddr,(uint8_t)n);
      return readRev(b,n);
   } // readFromRev

   // Typically used for big-endian addresses
   int writeToRev (const uint8_t devAddr, const uint8_t b[], int n)
   {
      if (n <= 0) { return(0); }
      I2C.beginTransmission(devAddr);
      n= writeRev(b,n);
      int r= I2C.endTransmission();
      if (0 == r) { return(n); } //else
      return(-r);
   } // writeToRev

   // Useful for transactions requiring big-endian address followed by arbitrary byte data
   int writeToRevThenFwd (const uint8_t devAddr, const uint8_t bR[], const int nR, const uint8_t bF[], const int nF)
   {
      if ((nR < 0) || (nF < 0)) { return(0); }
      I2C.beginTransmission(devAddr);
      int n= writeRev(bR,nR);
      n+= write(bF,nF);
      int r= I2C.endTransmission();
      if (0 == r) { return(n); } //else
      return(-r);
   } // writeToRevThenFwd

}; // CCommonI2CX1


#endif // CCOMMON_I2C_HPP
