/**
 * @file xy6020l.cpp
 * @brief UART control access to XY6020L DCDC
 *
 * This library provides an embedded and simplified ModBus implementation.
 * The data type of interfaces to physical values are in decimal, scaled almost in 0.01.
 *
 * Tested only on a Arduino Pro Micro clone from China
 * 
 * @author Jens Gleissberg
 * @date 2024
 * @license GNU Lesser General Public License v3.0 or later
 */

#include "Arduino.h"
#include "xy6020l.h"

/**
 * @brief: debug message level on serial
 * > 1 - timeout comment
 * > 2 - HRegs contents
 *     - rx bytes and HReg of written HReg
 * > 9 - rx bytes (basic RS232 communication)
 */
#define __debug__ 10

/** @brief period for reading content of all hold regs, in msec  */
#define PERIOD_READ_ALL_HREGS 200
/** @brief answer timeout of tx message: 7 x 10 ms */
#define PERIOD_TIMEOUT_RESPONSE 7 

xy6020l::xy6020l(Stream& serial, byte add)
{
  mSerial= &serial;
  mAdr=add;
  mRxBufIdx =  0;
  mRxFrameCnt=0; 
  mTxBufIdx =  0;
  mResponse = None;
  mTs = millis();
  mTO = mTs;
  mTLastTx = mTs;
};

bool xy6020l::RxDecode03( byte cnt)
{
  bool RxOk= true;
  char tmpBuf[30];

  mRxThis = mRxBuf[0]==mAdr? true:false;
  mRxSize = mRxBuf[2];
  if (mRxSize > NB_HREGS*2+5) 
  {
    RxOk= false;
  }
  else
  {
    for(int i=0; i< mRxSize/2; i++)
      hRegs[i]= (word)mRxBuf[3+2*i] * 256 + (word)mRxBuf[4+2*i];
  };
  // ignore CRC
  mRxFrameCnt++;
  
  #if __debug__ > 2  
  sprintf( tmpBuf, "%d: ", mRxBuf[1]);
  Serial.print(tmpBuf);
  for(int i=0; i < NB_HREGS; i++)
  {
    sprintf( tmpBuf, "%X(%4d) ", i, hRegs[i]);
    Serial.print(tmpBuf);
  }
  Serial.print("\n");
  #endif
  
  mResponse = None;
  return RxOk;
}

bool xy6020l::RxDecode06( byte cnt)
{
  bool RxOk= true;
  word RegNr;
  char tmpBuf[30];

  #if __debug__ > 2  
  for(int i=0; i < cnt; i++)
  {
    sprintf( tmpBuf, "%02X ",mRxBuf[i] );
    Serial.print(tmpBuf);
  }
  Serial.print("\n");
  #endif

  mRxThis = mRxBuf[0]==mAdr? true:false;
  if(mRxBuf[1]==0x06)
  {
    RegNr = (word)mRxBuf[2] *256 + mRxBuf[3];
    if (RegNr < NB_HREGS) 
    {
      hRegs[RegNr] = (word)mRxBuf[4] *256 + mRxBuf[5];
    }
  };
  
  #if __debug__ > 2  
  sprintf( tmpBuf, "%d %d:=%4d\n", mRxBuf[1], RegNr, hRegs[RegNr]);
  Serial.print(tmpBuf);
  #endif

  mResponse = None;
  return RxOk;
}

void xy6020l::task()
{
  char tmpBuf[30];

  // check rx buffer
  while ( (mSerial->available() > 0) && (mRxBufIdx < sizeof(mRxBuf)) ) 
  {
    mRxBuf[ mRxBufIdx++]= mSerial->read();
    #if __debug__ > 9  
    Serial.print("r");
    #endif
    // @todo optimize delay time in dependency from baudrate
    delayMicroseconds(1000);
  }
  if(mRxBufIdx > 7)
  {
    // mbus as different answer layouts
    switch(mRxBuf[0x01] )
    {
      case 0x3: RxDecode03(mRxBufIdx); break;
      case 0x6: RxDecode06(mRxBufIdx); break;
    }
  }
  mRxBufIdx=0;

  // transmits pending ?
  if(mResponse== None)
  {
    // transmit pause time:  90 ms !
    if (millis() > mTLastTx + 90) 
    {
      mTLastTx = millis();
    
      #if __debug__ > 9  
      Serial.print("T: ");
      for(int i=0; i < mTxBufIdx; i++)
      {
        sprintf( tmpBuf, "%02X ",mTxBuf[i] );
        Serial.print(tmpBuf);
      }
      Serial.print("\n");
      #endif

      if(mTxBufIdx > 0)
      {
        mSerial->write( mTxBuf, mTxBufIdx);
        mTxBufIdx=0;
        mResponse = Data;
      }
    }
    else 
    {
      // update all HReg 
      if (millis() > mTs + PERIOD_READ_ALL_HREGS) 
      {
        mTs = millis();
        SendReadHReg(0, 0x1E );
      }
    }
  }

  // answer timeout detection
  if (millis() > mTO + 10) 
  {
    mTO = millis();
    if(mResponse == None)
      mCntTO= PERIOD_TIMEOUT_RESPONSE;
    else
    {
      if(mCntTO>0) 
        mCntTO--;
      else
      {
        // TIME OUT
        #if __debug__ > 1 
        Serial.print("\n\n - -  TIMEOUT - - \n");
        //delay(10000);
        #endif
        mResponse= None;
      };
    }
  }
}

void xy6020l::SendReadHReg( word startReg, word endReg)
{
  // tx buffer free?
  if( mTxBufIdx == 0 )
  {
    mTxBuf[0]= mAdr;
    mTxBuf[1]= 0x03;
    mTxBuf[2]= startReg >> 8;
    mTxBuf[3]= startReg & 0xFF;    
    mTxBuf[4]= endReg >> 8;
    mTxBuf[5]= endReg & 0xFF;    
    CRCModBus(6);
    mTxBufIdx=8;
  }
}

bool xy6020l::setHReg(byte nr, word value)
{
  bool retVal=false;

  // tx buffer free?
  if(mTxBufIdx == 0) 
  {
    mTxBuf[0]= mAdr;
    mTxBuf[1]= 0x06;
    mTxBuf[2]= 0;
    mTxBuf[3]= nr;    
    mTxBuf[4]= value >> 8;
    mTxBuf[5]= value & 0xFF;    
    CRCModBus(6);
    mTxBufIdx=8;
    retVal= true;
  }
  return (retVal);
}

void xy6020l::CRCModBus(int datalen)
{
  word crc = 0xFFFF;
  for (int pos = 0; pos < datalen; pos++)
  {
      crc ^= mTxBuf[pos];

      for (int i = 8; i != 0; i--)
      {
          if ((crc & 0x0001) != 0)
          {
              crc >>= 1;
              crc ^= 0xA001;
          }
          else
              crc >>= 1;
      }
  }
  mTxBuf[datalen] = (byte)(crc & 0xFF);
  mTxBuf[datalen + 1] = (byte)((crc >> 8) & 0xFF);
}

bool xy6020l::setSlaveAdd( word add) 
{ 
  bool retVal= true;
  if( setHReg(HREG_IDX_SLAVE_ADD, add & (word)0x00FF ) )
  {
    // change address only if command could be places in tx buffer !
    //mAdr= add;
  }
  else
    retVal=false;

  return retVal;
};
