// Duino/AVR/PowMon.ino - Power monitoring / charge controller
// https://github.com/DrAl-HFS/Duino.git
// Licence: GPL V3A
// (c) Project Contributors July 2021



/***/

#define DEBUG      Serial
#define DEBUG_BAUD 115200

#define PIN_PULSE LED_BUILTIN // pin 13 = SPI CLK
#define PIN_CHARGE_ON 3

#define AVR_INTR_SLOW // reduce interrupt rate for more efficient low-speed control


/***/

#include "Common/AVR/DA_Timing.hpp"
#include "Common/AVR/DA_StrmCmd.hpp"
#include "Common/AVR/DA_Config.hpp"
#include "Charge.hpp"


CCharge gCharge;

CClock gClock(3000);

#if (2 == AVR_CLOCK_TIMER)
// Connect clock to timer interrupt
// SIGNAL blocks other ISRs - check use of sleep()
SIGNAL(TIMER2_COMPA_vect) { gClock.nextIvl(); }
#endif // AVR_CLOCK_TIMER

//CAnalogueDbg gADC;
ISR(ADC_vect) { gCharge.event(); }

StreamCmd gStreamCmd;
CmdSeg cmd; // Would be temp on stack but problems arise...


#define SEC_DIG 1
int8_t sysLog (Stream& s, uint8_t events)
{
  char str[96];
  int8_t m=sizeof(str)-1, n;
  
  n= gClock.getStrHMS(str,m);

  n+= gCharge.dumpS(str+n, m-n);

  s.println(str);
  s.flush();
  return(0);
} // sysLog

void setup (void)
{
  noInterrupts();
  
  //setID("Pro1" / "Mega1" / "Nano1" / "UnoV3");
  gClock.setA(__TIME__);
  gClock.start();
  gCharge.init(0); //gCharge.start();
  pinMode(PIN_PULSE, OUTPUT);
  pinMode(PIN_CHARGE_ON, OUTPUT);

  DEBUG.begin(DEBUG_BAUD);
  if (DEBUG)
  {
    char sid[8];
    if (getID(sid) > 0) { DEBUG.print(sid); }
    DEBUG.print(" PowMon " __DATE__ " ");
    DEBUG.println(__TIME__);
  }
  //
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();
  
  interrupts();
  gClock.intervalStart();
  sysLog(DEBUG,0);
} // setup

void pulseHack (void)
{ // Caveat: SPI CLK conflict
static uint16_t gHackCount= 0;
  if (((++gHackCount) & 0xFFF) >= 1000) { gHackCount-= 1000; }
  if ((gHackCount & 0xFFF) < 20)// && (0 == gSigGen.rwm))
  {
    //SPI.end(); 
    gHackCount|= 1<<15;
    digitalWrite(PIN_PULSE, HIGH);
  }
  else
  {
    digitalWrite(PIN_PULSE, LOW);
    //if ((0 != gSigGen.rwm) && (gHackCount & 1<<15)) { SPI.begin(); gHackCount &= 0xFFF; }
  }
} // pulseHack

void loop (void)
{
  uint8_t ev=0;
  if (gClock.update() > 0)
  { // ~100Hz update rate
    if (gClock.intervalUpdate()) { ev|= 0x80; }
    if (gStreamCmd.read(cmd,DEBUG))
    {
      ev|= 0x40;
      if (cmd.cmdF[0] & 0x10) // clock
      {
        uint8_t hms[3], d;
        d= cmd.v[SCI_VAL_MAX-1].extractHMS(hms,3);
        if (d >= 2)
        { 
          gClock.setHM(hms);
          if (d >= 3) { gClock.setS(hms[2]); }
          cmd.cmdR[0]|= 0x10;
        }
      }
    }
    if (ev)
    {
      sysLog(DEBUG,ev);
      if (ev & 0x40)
      {
        gStreamCmd.respond(cmd,DEBUG);
        cmd.clean();
      }
      gClock.intervalStart();
    }
    pulseHack();
    ev= digitalRead(PIN_CHARGE_ON);
    if (ev ^ gCharge.update(ev)) { digitalWrite(PIN_CHARGE_ON, ev^0x1); }
  }

  sleep_cpu();
} // loop
