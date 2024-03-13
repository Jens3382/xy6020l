/**
 * @file xy6020l.h
 * @brief UART control access to XY6020L DCDC
 *
 * This library provides an embedded and simplified ModBus implementation.
 * The data type of interfaces to physical values are in decimal, scaled almost in 0.01.
 *
 * Tested only on a Arduino Pro Micro clone from China
 *
 * Special thanks to user g-radmac for his discovery of the UART protocol!
 * References: 
 *   https://forum.allaboutcircuits.com/threads/exploring-programming-a-xy6020l-power-supply-via-modbus.197022/
 *   https://www.simplymodbus.ca/FAQ.htm
 *   
 * 
 * @author Jens Gleissberg
 * @date 2024
 * @license GNU Lesser General Public License v3.0 or later
 */

#ifndef xy6020l_h
#define xy6020l_h

#include "Arduino.h"

// the XY6020 provides 31 holding registers
#define NB_HREGS 31

// Holding Register index
// set voltage
#define HREG_IDX_CV 0  
// set current
#define HREG_IDX_CC 1
// actual voltage  0,01 V
#define HREG_IDX_ACT_V 2
// actual current   0,01 A
#define HREG_IDX_ACT_C 3
// actual output power  0,1 W
#define HREG_IDX_ACT_P 4
// input voltage  0,01 V
#define HREG_IDX_IN_V 5
// output charge  0,001 Ah
#define HREG_IDX_OUT_CHRG 6
#define HREG_IDX_OUT_CHRG_HIGH 7
// output energy  0,001 Wh
#define HREG_IDX_OUT_ENERGY 8
#define HREG_IDX_OUT_ENERGY_HIGH 9
// on time  [h]   ??
#define HREG_IDX_ON_HOUR 0x0A
// on time  [min]  
#define HREG_IDX_ON_MIN 0x0B
// on time  [s]  
#define HREG_IDX_ON_SEC 0x0C
// temperature  0,1 °C / Fahrenheit ?
#define HREG_IDX_TEMP 0x0D
#define HREG_IDX_TEMP_EXD 0x0E
// key lock changes
#define HREG_IDX_LOCK 0x0F
#define HREG_IDX_PROTECT 0x10
#define HREG_IDX_CVCC 0x11
// output on
#define HREG_IDX_OUTPUT_ON 0x12
#define HREG_IDX_FC 0x13
#define HREG_IDX_MODEL 0x16
#define HREG_IDX_VERSION 0x17
#define HREG_IDX_SLAVE_ADD 0x18
#define HREG_IDX_BAUDRATE 0x19
#define HREG_IDX_TEMP_OFS 0x1A
#define HREG_IDX_TEMP_EXT_OFS 0x1B
#define HREG_IDX_MEMORY 0x1D



/**
 * @class xy6020l
 * @brief Class for controlling the XY6020L DCDC converter
 */
class xy6020l 
{
  public:
    /**
     * @brief Constructor requires an interface to serial port
     * @param serial Stream object reference (i.e., Serial1)
     * @param add slave address of the xy device, can be change by setSlaveAdd command
     */
    xy6020l(Stream& serial, byte add);
    /**
     * @brief Task method that must be called in loop() function of the main program cyclically.
     * It automatically triggers the reading of the Holding Registers each PERIOD_READ_ALL_HREGS ms.
     */
    void task();

    /// @name XY6020L application layer: HReg register access
    /// @{
    // 
    /** @brief voltage setpoint, LSB: 0.01 V , R/W  */
    word getCV(void) { return (word)hRegs[ HREG_IDX_CV]; };
    void setCV( word cv) { setHReg(HREG_IDX_CV, cv);};

    /** @brief constant current  setpoint, LSB: 0.01 A , R/W  */
    word getCC() { return (word)hRegs[ HREG_IDX_CC]; };
    void setCC(word cc) { setHReg(HREG_IDX_CC, cc);};

    /** @brief actual input voltage , LSB: 0.01 V, readonly  */
    word getInV() { return (word)hRegs[ HREG_IDX_IN_V ]; };
    /** @brief actual voltage at output, LSB: 0.01 V, readonly  */
    word getActV() { return (word)hRegs[ HREG_IDX_ACT_V]; };
    /** @brief actual current at output, LSB: 0.01 A, readonly  */
    word getActC() { return (word)hRegs[ HREG_IDX_ACT_C]; };
    /** @brief actual power at output, LSB: 0.01 W, readonly  */
    word getActP() { return (word)hRegs[ HREG_IDX_ACT_P]; };
    /** @brief actual charge from output, LSB: 0.001 Ah, readonly, with high word because not tested yet  */
    word getCharge() { return (word)hRegs[ HREG_IDX_OUT_CHRG ]  ; };
    /** @brief actual energy provided from output, LSB: 0.001 Wh, readonly, with high word because not tested yet  */
    word getEnergy() { return (word)hRegs[ HREG_IDX_OUT_ENERGY ]  ; };
    /** @brief actual output time, LSB: 1 h, readonly */
    word getHour() { return (word)hRegs[ HREG_IDX_ON_HOUR ]  ; };
    /** @brief actual output time, LSB: 1 min, readonly */
    word getMin() { return (word)hRegs[ HREG_IDX_ON_MIN ]  ; };
    /** @brief actual output time, LSB: 1 secs, readonly */
    word getSec() { return (word)hRegs[ HREG_IDX_ON_SEC ]  ; };

    /** @brief dcdc temperature, LSB: 0.1°C/F, readonly */
    word getTemp() { return (word)hRegs[ HREG_IDX_TEMP ]  ; };
    /** @brief external temperature, LSB: 0.1°C/F, readonly */
    word getTempExt() { return (word)hRegs[ HREG_IDX_TEMP_EXD ]; };

    /** @brief lock switch, true = on, R/W   */
    bool getLockOn() { return hRegs[ HREG_IDX_LOCK]>0?true:false; };
    bool setLockOn(bool onState) { return setHReg(HREG_IDX_LOCK, onState?1:0);};

    /** @brief lock switch, true = on, R/W   */
    word getProtect() { return hRegs[ HREG_IDX_PROTECT]; };
    bool setProtect(word state) { return setHReg(HREG_IDX_PROTECT, state );};

    /** @brief returns if CC is active , true = on, read only   */
    bool isCC() { return hRegs[ HREG_IDX_CVCC]>0?true:false; };
    /** @brief returns if CV is active , true = on, read only   */
    bool isCV() { return hRegs[ HREG_IDX_CVCC]<1?true:false; };

    /** @brief output switch, true = on, R/W   */
    bool getOutputOn() { return hRegs[ HREG_IDX_OUTPUT_ON]>0?true:false; };
    bool setOutput(bool onState) { return setHReg(HREG_IDX_OUTPUT_ON, onState?1:0);};

    /** @brief set the temperature unit to °C, read not implemended because no use  */
    bool setTempAsCelsius(void)  { return setHReg(HREG_IDX_FC, 0);};
    /** @brief set the temperature unit to Fahrenheit, read not implemended because no use  */
    bool setTempAsFahrenheit(void)  { return setHReg(HREG_IDX_FC, 1);};

    /** @brief returns the product number, readonly */
    word getModel(void)  { return (word)hRegs[ HREG_IDX_MODEL ]  ; };
    /** @brief returns the version number, readonly */
    word getVersion(void)  { return (word)hRegs[ HREG_IDX_VERSION ]  ; };

    /** @brief slave address, R/W, take effect after reset of XY6020L !  */
    word getSlaveAdd(void) { return (word)hRegs[ HREG_IDX_SLAVE_ADD]; };
    bool setSlaveAdd( word add);

    /** @brief baud rate , W, no read option because on use  
        @todo: provide enum for rate number to avoid random/unsupported number */
    bool setBaudrate( word rate) { return setHReg(HREG_IDX_BAUDRATE, rate);};

    /** @brief internal temperature offset, R/W  */
    word getTempOfs(void) { return (word)hRegs[ HREG_IDX_TEMP_OFS]; };
    bool setTempOfs( word tempOfs) { return setHReg(HREG_IDX_TEMP_OFS, tempOfs );};

    /** @brief external temperature offset, R/W  */
    word getTempExtOfs(void) { return (word)hRegs[ HREG_IDX_TEMP_EXT_OFS]; };
    bool setTempExtOfs( word tempOfs) { return setHReg(HREG_IDX_TEMP_EXT_OFS, tempOfs );};

    /** @brief Presets, R/W  */
    word loadPreset(void) { return (word)hRegs[ HREG_IDX_MEMORY]; };
    bool savePreset( word preset) { return setHReg(HREG_IDX_MEMORY, preset );};

    /// @}


  private:
    byte          mAdr;
    Stream*       mSerial;
    byte            mRxBufIdx;
    unsigned char  mRxBuf[60];
    byte            mRxState;
    bool           mRxThis;
    byte            mRxSize;
    word           mRxFrameCnt;

    enum        Response { None, Confirm, Data };
    Response    mResponse;
    long        mTs;
    long        mTO;
    long        mTLastTx;
    byte        mCntTO;

    int            mTxBufIdx;
    unsigned char  mTxBuf[30];

    word           hRegs[NB_HREGS];

    bool setHReg(byte nr, word value);
    void CRCModBus(int datalen);
    bool RxDecode03( byte cnt);
    bool RxDecode06( byte cnt);
    void SendReadHReg( word startReg, word endReg);
};
#endif