# xy6020l

UART control access to XY6020L DCDC

The library implements a simplified ModBus implementation for data link layer. The physical values on application level are decimals and scaled almost in 0.01 within a word type.

Tested only on a Arduino Pro Micro clone from China

Special thanks to user g-radmac of allaboutcircuits forum for his discovery of the UART protocol! 

## References: 
- [@allaboutcircuits: Exploring programming a XY6020L power supply via modbus](https://forum.allaboutcircuits.com/threads/exploring-programming-a-xy6020l-power-supply-via-modbus.197022/)
- [Simply Modbus FAQ](https://www.simplymodbus.ca/FAQ.htm)

## Holding Registers

| Register | Content |
|-------|-----------|
| 0 | set voltage, LSB: 0.01 V|
| 1 | set current, LSB: 0.01 A |
| 2 | actual voltage, LSB: 0.01 V |
| 3 | actual current, LSB: 0.01 A |
| 4 | actual output power, LSB: 0.1 W |
| 5 | input voltage, LSB:  0.01 V |
| 6 | output charge, LSB:  0.001 Ah |
| 7 | output charge high, no checked! |
| 8 | output energy, LSB:  0.001 Wh |
| 9 | output energy high, no checked! |
| 10 | on time  [h] |
| 11 | on time  [min] |
| 12 | on time  [s] |
| 13 | temperature, LSB:  0.1 °C |
| 18 | output on |

## Example Application

Usage of XY6020L DCDC for simple max power point tracking of 
a solar module driving a electrolytic cell
Cells voltage start from ~3 V and current will rise up to ~3A at 4 V.

At ~19 V the 20V pannel has its maximum power.
Simple I-controler increases the cell voltage till the power consumption from the solar panel drives its voltage below 19 V.
    
Hardware:  Arduino Pro Micro Clone from China

## Class: xy6020l

Class for controlling the XY6020L DCDC converter

### Public Methods

- `xy6020l(Stream& serial)`: Constructor requires an interface to serial port
- `void task()`: Task method that must be called in loop() function of the main program cyclically. It automatically triggers the reading of the Holding Registers each PERIOD_READ_ALL_HREGS ms.

#### XY6020L application layer: HReg register access

- `word getCV(void)`: voltage setpoint, LSB: 0.01 V , R/W
- `void setCV( word cv)`: sets the voltage setpoint
- `word getCC()`: constant current setpoint, LSB: 0.01 A , R/W
- `void setCC(word cc)`: sets the constant current setpoint
- `bool getOutputOn()`: output switch, true = on, R/W
- `void setOutput(bool onState)`: sets the output switch state
- `word getInV()`: actual input voltage , LSB: 0.01 V, readonly
- `word getActV()`: actual voltage at output, LSB: 0.01 V, readonly
- `word getActC()`: actual current at output, LSB: 0.01 A, readonly
- `word getActP()`: actual power at output, LSB: 0.01 W, readonly
- `word getCharge()`: actual charge from output, LSB: 0.001 Ah, readonly, with high word because not tested yet
- `word getEnergy()`: actual energy provided from output, LSB: 0.001 Wh, readonly, with high word because not tested yet
- `word getHour()`: actual output time, LSB: 1 h, readonly
- `word getMin()`: actual output time, LSB: 1 min, readonly
- `word getSec()`: actual output time, LSB: 1 secs, readonly
- `word getTemp()`: dcdc temperature, LSB: 0.1°C, readonly

## copyrights:

Author: Jens Gleissberg
Date: 2024
License: GNU Lesser General Public License v3.0 or later
