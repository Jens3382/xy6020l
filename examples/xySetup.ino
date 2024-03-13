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
xy6020l xy(Serial1, 0x01);

long ts;
int task;

void setup() {
  // debug messages via USB
  Serial.begin( 115200);
  // MBus serial
  Serial1.begin( 115200);

  ts = millis();
  task=0;
}

void loop() {
  int vDiff;
  char tmpBuf[30];  // text buffer for serial messages
  word tmpW1, tmpW2;
  
  xy.task();
  
  // 500 ms  control period
  if (millis() > ts + 500) 
  {
    ts = millis();

    switch( task)
    {
      case 4:
        sprintf( tmpBuf, "\nM:%04X V:%04X\n", xy.getModel(), xy.getVersion() );
        Serial.print(tmpBuf);
        break;
      case 6: 
        if(!xy.setTempAsCelsius())
          Serial.print("\nset temp to °C failed\n");
        else
          Serial.print("\nset temp to °C requested\n");
        break;
      case 9:
        if(!xy.setSlaveAdd(1) )
          Serial.print("\nset setSlaveAdd failed\n");
        else
          Serial.print("\nset setSlaveAdd requested\n");
        break;
    }
    task++;
  }
}
