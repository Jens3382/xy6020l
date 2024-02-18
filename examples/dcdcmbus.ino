/**
 * @file dcdcmbus.ino
 * @brief usage of XY6020L DCDC for simple max power point tracking of 
 *  a solar module driving a electrolytic cell
 *  Cells voltage start from ~3 V and current will rise up to ~3A at 4 V.
 *  At ~19 V the 20V pannel has its maximum power.
 *  Simple I-controler increases the cell voltage till the power consumption
 *  from the solar panel drives its voltage below 19 V.
    Hardware:  Arduino Pro Micro Clone from China
 * 
 * @author Jens Gleissberg
 * @date 2024
 * @license GNU Lesser General Public License v3.0 or later
 */

#include "xy6020l.h"

// dcdc's MBus is connected to Serial1 of Arduino
xy6020l xy(Serial1);

long ts;
bool  boActive;
// solar panel voltage
word  vIn;
// cell voltage setpoint
word  vOut, vOutMin, vOutMax;

void setup() {
  // debug messages via USB
  Serial.begin( 115200);
  // MBus serial
  Serial1.begin( 115200);

  ts = millis();

  vOutMin= 250; // start with 2,5 V
  vOutMax= 450; // max voltage
  vIn = 1700; 
  boActive = false;
}

void loop() {
  int vDiff;
  char tmpBuf[30];  // text buffer for serial messages

  xy.task();
  
  // 500 ms  control period
  if (millis() > ts + 500) 
  {
    ts = millis();

    vIn = xy.getInV();
    // 15 V -> undervoltage of solar panel 
    if(vIn < 1500 )
    {
      // output off
      xy.setOutput(0);  
      boActive = false;
      // reset cell voltage to its min value
      vOut= vOutMin;
    }
    else 
    {
      // use a hyseressis for switch on to avoid to short on pulses
      if(vIn > 2100 )
      {
        boActive= true;
        // output on
        if(!xy.getOutputOn() )
          xy.setOutput(1);  
      }
    }

    if(boActive)
    {
        // target: 
        // the input voltage must be kept at 19 V
        // @todo I part depents on voltage difference and slope
        vDiff= (int)vIn - (int)1900;
        if(vDiff > 200)
          vDiff=200;
        if(vDiff < 200)
          vDiff=-200;

        if(vDiff < 0)
          vOut+= vDiff;
        else
          vOut+= vDiff /10;

        if(vOut > vOutMax)
          vOut = vOutMax;
        if(vOut < vOutMin)
          vOut = vOutMin;
        xy.setCV( vOut );
    }

    // print control results:
    sprintf( tmpBuf, "%d: %d   Out=%d\n", vIn, vOut, boActive);
    Serial.print(tmpBuf);
  }
}
