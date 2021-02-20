// Duino/STM32/BlinkST/morsePattern.h - International Morse Code bit (pulse) patterns
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors Feb 2021

#ifndef MORSE_PATTERN_H
#define MORSE_PATTERN_H

#if 0 //def __cplusplus
extern "C" {
#endif

// International Morse Code (up to) 5 pulse/bit patterns packed into uint8_t 
// 3b MSB #, 5b lsb code (sent msb first)
#define IMC5_N_SHIFT 5
#define IMC5_C_MASK ((1<<IMC5_N_SHIFT)-1)
// declaration macro
#define IMC5(n,c) ( ((n) << IMC5_N_SHIFT) | ((c) & IMC5_C_MASK) )

// bad form to declare in a header...
//extern
const uint8_t gAlphaIMC5[]=
{
  IMC5(2, 0b01), IMC5(4, 0b1000), IMC5(4, 0b1010), IMC5(3, 0b100),  // A B C D
  IMC5(1, 0b0), IMC5(4 ,0b0010), IMC5(3 ,0b110), IMC5(4 ,0b0000),   // E F G H
  IMC5(2 ,0b00), IMC5(4 ,0b0111), IMC5(3 ,0b101), IMC5(4 ,0b0100),  // I J K L
  IMC5(2 ,0b11), IMC5(2 ,0b10), IMC5(3 ,0b111), IMC5(4 ,0b0110),    // M N O P
  IMC5(4 ,0b1101), IMC5(3 ,0b010), IMC5(3 ,0b000), IMC5(1 ,0b1),    // Q R S T
  IMC5(3 ,0b001), IMC5(4 ,0b0001), IMC5(3 ,0b011), IMC5(4 ,0b1001), // U V W X
  IMC5(4 ,0b1011), IMC5(4 ,0b1100)   // Y Z
}; // gAlphaIMC5
//extern
const uint8_t gNumIMC5[]=
{
  IMC5(5, 0b11111), IMC5(5, 0b01111), IMC5(5, 0b00111), IMC5(5, 0b00011),   // 0 1 2 3
  IMC5(5, 0b00001), IMC5(5, 0b00000), IMC5(5, 0b10000), IMC5(5, 0b11000),   // 4 5 6 7
  IMC5(5, 0b11100), IMC5(5, 0b11110)  // 8 9
}; // gNumIMC5

// Relative duration of dot, dash, inter-symbol gap, inter-word gap
// TODO: Review hackily merged pulse & gap - should properly be
// declared as { {1,3,7}, {1,3} } but this is cumbersome at present...)
// Inter-pulse gap assumed to be same duration as dot
//extern
const uint8_t gTimeRelIMC[]={1,3,3,7};

// extern
int8_t unpackIMC5 (uint8_t code[], const uint8_t imc5)
{
  code[0]= imc5 & IMC5_C_MASK;
  return(imc5>>IMC5_N_SHIFT); 
}

#if 0 //def __cplusplus
} // extern "C"
#endif

#endif // MORSE_PATTERN_H
