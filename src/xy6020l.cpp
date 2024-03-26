// #include <type_traits>
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
#define __debug__ 0

/** @brief period for reading content of all hold regs, in msec  */
// #define PERIOD_READ_ALL_HREGS 100
/** @brief answer timeout of tx message: 4 x 10 ms */
#define PERIOD_TIMEOUT_RESPONSE 4 



TxRingBuffer::TxRingBuffer()
{
  mpIn = &(mTxBuf[0]);
  mpOut= &(mTxBuf[0]);
  mIn=0;
}

bool TxRingBuffer::AddTx(txRingEle* pTxEle)
{
  bool retVal=true;
  char tmpBuf[20];

  if( IsFull())
    retVal= false;
  else
  {
    mpIn->mHregIdx = pTxEle->mHregIdx;  
    mpIn->mValue   = pTxEle->mValue;
    mIn++;
    mpIn++;
    // wrap around
    if(mpIn == &(mTxBuf[TX_RING_BUFFER_SIZE]))
      mpIn = &(mTxBuf[0]);

    sprintf( tmpBuf, "\nRing In: %d\n", mIn);
    Serial.print(tmpBuf);
  }
  return retVal;
}

bool TxRingBuffer::AddTx(byte hRegIdx, word value)
{
  txRingEle TxEle;
  TxEle.mHregIdx= hRegIdx;
  TxEle.mValue  = value;
  return AddTx( &TxEle );
}

bool TxRingBuffer::GetTx(txRingEle& TxEle)
{
  bool retVal=true;
  if( IsEmpty() )
    retVal= false;
  else
  {
    TxEle.mHregIdx = mpOut->mHregIdx;
    TxEle.mValue = mpOut->mValue;
    mIn--;
    mpOut++;
    // wrap around
    if(mpOut == &(mTxBuf[TX_RING_BUFFER_SIZE]))
      mpOut = &(mTxBuf[0]);
  };
  return retVal;
}


xy6020l::xy6020l(Stream& serial, byte adr, byte txPeriod, byte options )
{
  mSerial= &serial;
  mAdr=adr;
  mOptions = options;
  mMemory = 255;
  mMemoryState= Send;
  mRxBufIdx =  0;
  mRxFrameCnt=0; 
  mRxFrameCntLast=0;
  mTxBufIdx =  0;
  mTxPeriod  = txPeriod;
  mResponse = None;
  mTs = millis();
  mTO = mTs;
  mTLastTx = mTs;
};

bool xy6020l::HRegUpdated(void)
{
  bool retValue=false;
  if(  mRxFrameCnt != mRxFrameCntLast)
  {
    mRxFrameCntLast= mRxFrameCnt;
    retValue=true;
  }
  return retValue;
};

/** @brief: read register reply, must be decoded to Hregister content */
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
    {
      if(mMemory > 10 )
      {
        hRegs[i]= (word)mRxBuf[3+2*i] * 256 + (word)mRxBuf[4+2*i];
      }
      else
      {
        mMem[i]= (word)mRxBuf[3+2*i] * 256 + (word)mRxBuf[4+2*i];
      }
    }
  };
  // ignore CRC
  mRxFrameCnt++;
  
  #if __debug__ > 2  
  sprintf( tmpBuf, "\nDec03:%d: ", mRxBuf[1]);
  Serial.print(tmpBuf);
  #endif
  #if __debug__ > 9  
  if(mMemory > 10 )
  {
    for(int i=0; i < NB_HREGS; i++)
    {
      sprintf( tmpBuf, "%X(%4d) ", i, hRegs[i]);
      Serial.print(tmpBuf);
    }
  } 
  else 
  {
    for(int i=0; i < NB_MEMREGS; i++)
    {
      sprintf( tmpBuf, "%X(%4d) ", i, mMem[i]);
      Serial.print(tmpBuf);
    }
  }
  Serial.print("\n");
  #endif

  // reset memory redirection
  mMemory=255;
  
  mResponse = None;
  return RxOk;
}

bool xy6020l::RxDecode06( byte cnt)
{
  bool RxOk= true;
  word RegNr;
  char tmpBuf[30];

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
  sprintf( tmpBuf, "Dec06: %d %d:=%4d\n", mRxBuf[1], RegNr, hRegs[RegNr]);
  Serial.print(tmpBuf);
  #endif

  mResponse = None;
  return RxOk;
}

bool xy6020l::RxDecode16( byte cnt)
{
  bool RxOk= true;
  word RegNr;
  char tmpBuf[30];

  mRxThis = mRxBuf[0]==mAdr? true:false;
  if(mRxBuf[1]==0x10)
  {
    RegNr = (word)mRxBuf[2] *256 + mRxBuf[3];
  };
  
  #if __debug__ > 2  
  sprintf( tmpBuf, "\nDec16: %d RegStart: 0x%X\n", mRxBuf[1], RegNr);
  Serial.print(tmpBuf);
  #endif

  mResponse = None;
  return RxOk;
}

void xy6020l::RxDecodeExceptions(byte cnt)
{
  char tmpBuf[30];

  if(cnt >= 5)
  {
    mLastExceptionCode = mRxBuf[2];
    // reset memory redirection
    mMemory=255;
    mResponse = None;

    #if __debug__ > 2  
    sprintf( tmpBuf, "\nException to fct code %d", ( mRxBuf[1] & 0x0F) );
    Serial.print(tmpBuf);
    switch( mRxBuf[2])
    {
      case 1: Serial.print(F("\nIllegal Function")); break;
      case 2: Serial.print(F("\nIllegal Data Address")); break;
      case 3: Serial.print(F("\nIllegal Data Value")); break;
      case 4: Serial.print(F("\nSlave Device Failure")); break;
      case 5: Serial.print(F("\nAcknowledge")); break;
      case 6: Serial.print(F("\nSlave Device Busy")); break;
      case 7: Serial.print(F("\nNegative Acknowledge")); break;
      case 8: Serial.print(F("\nMemory Parity Error")); break;
      case 10:Serial.print(F("\nGateway Path Unavailable")); break;
      case 11:Serial.print(F("\nGateway Target Device Failed to Respond")); break;
      default:Serial.print(F("\nUnknown Exception Code")); break;
    }
    #endif
  }
}

void xy6020l::task()
{
  char tmpBuf[30];

  mRxBufIdx=0;
  // check rx buffer
  #if __debug__ > 9  
  if(mSerial->available() > 0)
    Serial.print("\nRX: ");
  #endif

  while ( (mSerial->available() > 0) && (mRxBufIdx < sizeof(mRxBuf)) ) 
  {
    mRxBuf[ mRxBufIdx] = mSerial->read();

    #if __debug__ > 9  
    sprintf( tmpBuf, "%02X ", mRxBuf[ mRxBufIdx]);
    Serial.print(tmpBuf);
    #endif

    mRxBufIdx++;
    // @todo optimize delay time in dependency from baudrate
    delayMicroseconds(1000);
  }
  if(mRxBufIdx > 7)
  {
    if(mRxBuf[1] & 0x80)
      RxDecodeExceptions(mRxBufIdx);
    else
    {
      // mbus as different answer layouts
      switch(mRxBuf[1] )
      {
        case 0x3: RxDecode03(mRxBufIdx); break;
        case 0x6: RxDecode06(mRxBufIdx); break;
        case 0x10:RxDecode16(mRxBufIdx); break;
      }
    }
  }
  // transmits pending ?
  if(mResponse== None)
  {
    // response received -> tx next after pause time
    // transmit pause time:  mTxPeriod ms !
    if (millis() > mTLastTx + mTxPeriod) 
    {
      // something in the txbuffer ?  -> send Tx data out
      if(mTxBufIdx > 0)
      {

          #if __debug__ > 9  
          Serial.print("Send bytes: ");
          for(int i=0; i < mTxBufIdx; i++)
          {
            sprintf( tmpBuf, "%02X ",mTxBuf[i] );
            Serial.print(tmpBuf);
          }
          Serial.print("\n");
          #endif

        mSerial->write( mTxBuf, mTxBufIdx);
        mTxBufIdx=0;
        mResponse = Data;
        mTLastTx = millis();  // wait from here PERIOD_TX_PAUSE for next transmits 
        #if __debug__ > 2  
        Serial.print("Tx Buf send\n");
        #endif

      }
      else
      {
        // prioritize queued register writes against updating Hregs 

        // any command queued ?
        if( !mTxRingBuffer.IsEmpty() ) 
        {
          setHRegFromBuf();
        }
        else 
        {
          // update all HReg 
          if(!(mOptions & XY6020_OPT_NO_HREG_UPDATE))
            SendReadHReg(0, NB_HREGS-1 );

        }
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
        Serial.print(F("\n\n - -  TIMEOUT - - *********************************************************************************************************\n"));
        //delay(10000);
        #endif
        mResponse= None;
        // reset memory redirection
        mMemory=255;
        // dummy increment frame counter to release any blocked waiting loop
        mRxFrameCnt++;
      };
    }
  }
};

void xy6020l::SendReadHReg( word startReg, word nbRegs)
{
  // tx buffer free?
  if( mTxBufIdx == 0 )
  {
    mTxBuf[0]= mAdr;
    mTxBuf[1]= 0x03;
    mTxBuf[2]= startReg >> 8;
    mTxBuf[3]= startReg & 0xFF;    
    mTxBuf[4]= nbRegs >> 8;
    mTxBuf[5]= nbRegs & 0xFF;    
    CRCModBus(6);
    mTxBufIdx=8;
  }
}

void xy6020l::SetMemory(tMemory& mem )
{
  if( mem.Nr<10)
  {
    mMemory= mem.Nr;
    /** @todo:  check memcpy for fast copy */
    mMem[HREG_IDX_M_VSET] = mem.VSet;
    mMem[HREG_IDX_M_ISET] = mem.ISet;
    mMem[HREG_IDX_M_SLVP] = mem.sLVP;
    mMem[HREG_IDX_M_SOVP] = mem.sOVP;
    mMem[HREG_IDX_M_SOCP] = mem.sOCP;
    mMem[HREG_IDX_M_SOPP] = mem.sOPP;
    mMem[HREG_IDX_M_SOHPH]= mem.sOHPh;
    mMem[HREG_IDX_M_SOHPM]= mem.sOHPm;
    mMem[HREG_IDX_M_SOAHL]= (word)(mem.sOAH & 0xFFFF);
    mMem[HREG_IDX_M_SOAHH]= (word)(mem.sOAH >> 16);
    mMem[HREG_IDX_M_SOWHL]= (word)(mem.sOWH & 0xFFFF);
    mMem[HREG_IDX_M_SOWHH]= (word)(mem.sOWH >> 16);
    mMem[HREG_IDX_M_SOTP]= mem.sOTP;
    mMem[HREG_IDX_M_SINI]= mem.sINI;
    // queue cmd for memory write 
    // only 1 memory write at 1 time !
    mTxRingBuffer.AddTx( HREG_IDX_M0 + mem.Nr * HREG_IDX_M_OFFSET, 0 );
  }
}

bool xy6020l::GetMemory(tMemory* pMem)
{
  bool retVal= false;
  switch(mMemoryState)
  {
    case Send:
      if(pMem!= nullptr && (pMem->Nr < 10) )
      {
        mMemory= pMem->Nr;
        SendReadHReg( HREG_IDX_M0 + pMem->Nr * HREG_IDX_M_OFFSET, NB_MEMREGS);
        mMemoryState = Wait;
        mMemoryLastFrame= mRxFrameCnt;
      }
      break;
    case Wait:
      if( mMemoryLastFrame != mRxFrameCnt)
      {
        // 
        pMem->VSet = mMem[HREG_IDX_M_VSET];
        pMem->ISet = mMem[HREG_IDX_M_ISET];
        pMem->sLVP = mMem[HREG_IDX_M_SLVP];
        pMem->sOVP = mMem[HREG_IDX_M_SOVP];
        pMem->sOCP = mMem[HREG_IDX_M_SOCP];
        pMem->sOPP = mMem[HREG_IDX_M_SOPP];
        pMem->sOHPh= mMem[HREG_IDX_M_SOHPH];
        pMem->sOHPm= mMem[HREG_IDX_M_SOHPM];
        pMem->sOAH = mMem[HREG_IDX_M_SOAHL] | ((unsigned long)mMem[HREG_IDX_M_SOAHH])<<16;
        pMem->sOWH = mMem[HREG_IDX_M_SOWHL] | ((unsigned long)mMem[HREG_IDX_M_SOWHH])<<16;
        pMem->sOTP = mMem[HREG_IDX_M_SOTP];
        pMem->sINI = mMem[HREG_IDX_M_SINI];        

        mMemory = 255;
        mMemoryState= Send;
        retVal = true;
      }
      break;
  }
  return retVal;
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

bool xy6020l::setHRegFromBuf()
{
  bool retVal=false;
  txRingEle txEle;
  int iMem;

  // buffer filled ?
  if( !mTxRingBuffer.IsEmpty()) 
  {
    if(mTxRingBuffer.GetTx(txEle))
    {
      // check if register needs to be updated at all  -> skip transfer to reduce update period of HRegs
      if( !( mOptions & XY6020_OPT_SKIP_SAME_HREG_VALUE ) ||
           ( hRegs[txEle.mHregIdx] !=  txEle.mValue ) )
      {
        mTxBuf[0]= mAdr;
        mTxBuf[1]= 0x06;
        mTxBuf[2]= 0;
        mTxBuf[3]= txEle.mHregIdx;    
        if(txEle.mHregIdx < HREG_IDX_M0)
        {
          // "normal" hregs
          mTxBuf[4]= txEle.mValue >> 8;
          mTxBuf[5]= txEle.mValue & 0xFF;    
          CRCModBus(6);
          mTxBufIdx=8;
        }
        else {
          // memory set HRegs
          setMemoryRegs(txEle.mHregIdx);
        }
      }
      else 
      {
        #if __debug__ > 2  
        Serial.print(F("\nSkip HReg Update!\n"));
        #endif
      }
      // else { send nothing }; 
      retVal= true;
    }
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

void xy6020l::setMemoryRegs(byte HRegIdx)
{
  int iMem;

  // buffer filled ?
  mTxBuf[0]= mAdr;
  mTxBuf[1]= 16;
  // start address
  mTxBuf[2]= 0;
  mTxBuf[3]= HRegIdx;    
  // number of regs to write
  mTxBuf[4]=  0;
  mTxBuf[5]= 14;
  // bytes to write
  mTxBuf[6]= 2* 14;
  // memory set HRegs
  // register are cached in mMem[] array !
  for(iMem=0; iMem<NB_MEMREGS; iMem++)
  {
    mTxBuf[7+iMem*2]= mMem[iMem] >> 8;
    mTxBuf[8+iMem*2]= mMem[iMem] & 0xFF;    
  }
  CRCModBus(9+14*2);
  mTxBufIdx=11+14*2;
}

void xy6020l::PrintMemory(tMemory& mem)
{
  char tmpBuf[50];
  Serial.print(F("\nList Memory Content:"));

  sprintf( tmpBuf, "\nNr: %d ", mem.Nr );   Serial.print(tmpBuf);
  sprintf( tmpBuf, "\nV-SET = %d (Voltage setting)", mem.VSet );   Serial.print(tmpBuf);
  sprintf( tmpBuf, "\nI-SET = %d (Current setting)", mem.ISet );   Serial.print(tmpBuf);
  sprintf( tmpBuf, "\nS-LVP = %d (Low voltage protection value)", mem.sLVP );   Serial.print(tmpBuf);
  sprintf( tmpBuf, "\nS-OVP = %d (Overvoltage protection value)", mem.sOVP );   Serial.print(tmpBuf);
  sprintf( tmpBuf, "\nS-OCP = %d (Overcurrent protection value)", mem.sOCP );   Serial.print(tmpBuf);
  sprintf( tmpBuf, "\nS-OPP = %d (Over power protection value)", mem.sOPP );   Serial.print(tmpBuf);
  sprintf( tmpBuf, "\nS-OHP_H = %d (Maximum output time - hours)", mem.sOHPh );   Serial.print(tmpBuf);
  sprintf( tmpBuf, "\nS-OHP_M = %d (Maximum output time - minutes)", mem.sOHPm );   Serial.print(tmpBuf);
  sprintf( tmpBuf, "\nS-OAH = %d (Maximum output charge Ah)", mem.sOAH );   Serial.print(tmpBuf);
  sprintf( tmpBuf, "\nS-OWH = %d (Maximum output energy Wh)", mem.sOWH );   Serial.print(tmpBuf);
  sprintf( tmpBuf, "\nS-OTP = %d (Over temperature protection)", mem.sOTP );   Serial.print(tmpBuf);
  sprintf( tmpBuf, "\nS-INI = %d (Power-on output switch)", mem.sINI );   Serial.print(tmpBuf);

  Serial.print(F("\n"));
}
