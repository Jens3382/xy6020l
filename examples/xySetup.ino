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
xy6020l xy(Serial1, 0x01, 50, XY6020_OPT_SKIP_SAME_HREG_VALUE | XY6020_OPT_NO_HREG_UPDATE);

long ts;
int task;
byte MemIdx;

void setup() {
  // debug messages via USB
  Serial.begin( 115200);
  // MBus serial
  Serial1.begin( 115200);

  ts = millis();
  task=0;
  MemIdx = 0;
}

void loop() {
  int vDiff;
  char tmpBuf[30];  // text buffer for serial messages
  word tmpW1, tmpW2;
  tMemory Mem;
  
  xy.task();
  
  // 100 ms  control period
  if (millis() > ts + 500) 
  {
    ts = millis();

    sprintf( tmpBuf, "\nTask: %d ", task );
    Serial.print(tmpBuf);

    switch( task)
    {
      case 4:
        xy.ReadAllHRegs();
        while(!xy.HRegUpdated())
          xy.task();
        sprintf( tmpBuf, "\nM:%04X V:%04X\n", xy.getModel(), xy.getVersion() );
        Serial.print(tmpBuf);
        task++;
        break;
      case 6: 
        if(!xy.setTempAsCelsius())
          Serial.print("\nset temp to °C failed\n");
        else
          Serial.print("\nset temp to °C requested\n");
        task++;
        break;
      /*case 9:
        if(!xy.setSlaveAdd(1) )
          Serial.print("\nset setSlaveAdd failed\n");
        else
          Serial.print("\nset setSlaveAdd requested\n");
        task++;
        break;*/
      case 8:
        // LiPo charger
        Mem.Nr = 0;
        Mem.VSet =  410;
        Mem.ISet =   50;
        Mem.sLVP = 1000;
        Mem.sOVP =  500;
        Mem.sOCP =  100;
        Mem.sOPP =   40;
        Mem.sOHPh= 0;
        Mem.sOHPm= 0;
        Mem.sOAH = 0;
        Mem.sOWH = 0;
        Mem.sOTP = 110;
        Mem.sINI = 0;
        xy.SetMemory(Mem);
        xy.PrintMemory(Mem);
        task++;
        break;
      case 9:
        // electrolytical cell
        Mem.Nr = 1;
        Mem.VSet =  300;
        Mem.ISet =  500;
        Mem.sLVP = 1000;
        Mem.sOVP =  920;
        Mem.sOCP =  420;
        Mem.sOPP =   40;
        Mem.sOHPh= 0;
        Mem.sOHPm= 0;
        Mem.sOAH = 0;
        Mem.sOWH = 0;
        Mem.sOTP = 110;
        Mem.sINI = 0;
        xy.SetMemory(Mem);
        xy.PrintMemory(Mem);
        task++;
        break;
      case 10:
        // 12 V supply
        Mem.Nr = 2;
        Mem.VSet = 1200;
        Mem.ISet =  500;
        Mem.sLVP = 1000;
        Mem.sOVP = 1300;
        Mem.sOCP =  620;
        Mem.sOPP = 1040;
        Mem.sOHPh= 0;
        Mem.sOHPm= 0;
        Mem.sOAH = 0;
        Mem.sOWH = 0;
        Mem.sOTP = 110;
        Mem.sINI = 0;
        xy.SetMemory(Mem);
        xy.PrintMemory(Mem);
        task++;
        break;
      case 11:
        Mem.Nr=MemIdx;
        if( xy.GetMemory(&Mem) )
        {
          xy.PrintMemory(Mem);
          MemIdx++;
          if(MemIdx > 6)
            task++;
        }
        break;
      default:
        task++;
    }
  }
}
