/*
 buses.h
 Serial SPI and I2C buses functions header file
 */

#ifndef _BUSES_MODULE
#define _BUSES_MODULE

// Gyroscope registers ------------------------------

// Device identification. Returns 212 (R)
#define GYR_WHO_AM_I   0x0F

// Control Reg 1 (RW)
#define GYR_CREG1  0x20
// Bits 7,6 : DR1,DR0 : Output data rate
// Bits 5,4 : BW1,BW0 : Bandwidth
// Bit    3 : PD      : Power-down (1 Active) Default is 0!
// Bit    2 : Zen     : Enable Z axis. Default is 1 (enabled)
// Bit    1 : Xen     : Enable X axis. Default is 1 (enabled)
// Bit    0 : Yen     : Enable Y axis. Default is 1 (enabled)

// Control Reg 2 (RW) High Pass filter
#define GYR_CREG2  0x21

// Control Reg 3 (RW) Interrupt control
#define GYR_CREG3  0x22

// Control Reg 4 (RW) Scale and communication
// By default Full Scale is +/-250 dps (Degrees/s)
// so each LSB corresponds to 8.75 mdps
// Zero rate level if for this scale +/-10 dps
// that corresponds to +/- 1100 LSBs aprox.
// We need then to set the zero level
#define GYR_CREG4  0x23

#define GYR_CREG5  0x24
#define GYR_REF    0x25

// Temperatura data in two's complement (R)
#define GYR_TEMP   0x26

// Status register (R)
#define GYR_STATUS 0x27

// Output registers in two's complement (R)
#define GYR_X_L    0x28
#define GYR_X_H    0x29
#define GYR_Y_L    0x2A
#define GYR_Y_H    0x2B
#define GYR_Z_L    0x2C
#define GYR_Z_H    0x2D

// There are more registers but they are not listed here

// SPI2 definitions ------------------------------------

#define N_SPI_CS    4  // Number of SPI Chip Selects 0..3

// I2C definitions ------------------------------------

typedef struct
  {
  uint16_t speed;
  uint32_t TIMINGR;
  } I2C_Speed_Data;

// Accelerometer information ------------------------------------
//
// By default the end of scale is +/-2g
// Manual indicates 1 mg/LSB for this scale but
// full scale is 32768 for 2g so that gives 61 ug/LSB

#define ACCEL_ADDR    0x19  // Accel I2C 7bit address

// Accel registers

// Control register 1
#define ACCEL_CTRL_REG1_A    0x20
//Bit 7..4 : Output Data Rate      As these bits default to 0000
//           0000 : Power Down     the system starts in Power Down
//           0001 : 1Hz
//           0010 : 10 Hz
//           0011 : 25 Hz
//           0100 : 50 Hz
//           0101 : 100 Hz
//           0110 : 200 Hz
//           0111 : 400 Hz
//           1000 : 1620 kHz
//           1001 : 1344 kHz / 5376 kHz
// Bit 3: LPEN : Low power enable (0 Normal/1 Low Power)
// Bit 2: Z Enable
// Bit 1: Y Enable
// Bit 0: X Enable

// Output registers for X,Y,Z Low and High Bytes
#define ACCE_OUT_XL  0x28
#define ACCE_OUT_XH  0x29
#define ACCE_OUT_YL  0x2A
#define ACCE_OUT_YH  0x2B
#define ACCE_OUT_ZL  0x2C
#define ACCE_OUT_ZH  0x2D

// The rest of registers are not considered in this program

// Magnetometer information -----------------------------
//
// Using gain setting GN=001
//    Range is +/- 1.3 gauss
//    Conversion is 1100 LSBs/gauss for X,Y
//                   980 LSBs/gauss for Z

#define MAGNET_ADDR    0x1E  // Magnet I2C 7bit address

#define MAG_CRA_REG_M  0x00
// Bit 7    : Temp enable : 0 (default) disabled / 1 enabled
// Bits 4..2: Output data rate
//               000 :0: 0,75 Hz
//               001 :1: 1,5 Hz
//               010 :2: 3,0 Hz
//               011 :3: 7,5 Hz
//               100 :4: 15 Hz (Default)
//               101 :5: 30 Hz
//               110 :6: 75 Hz
//               111 :7: 220 Hz
//
// Default value 00010000 : 15Hz
//
// The rest of bits must be set to zero

#define MAG_CRB_REG_M 0x01
// Bits 7..5 : Gain
//               Range     X,Y       Z
//       Code  +/-gauss LSB/gauss LSB/gauss
//      001:1    1.3      1100      980
//      010:2    1.9       855      760
//      011:3    2.5       670      600
//      100:4    4.0       450      400
//      101:5    4.7       400      355
//      110:6    5.6       330      295
//      111:7    8.1       230      205
//
// Default value: 0010000 : +/-1.3 gauss
//
// The rest of bits must be set to zero

#define MAG_MR_REG_M 0x02
// Bits 1,0: Mode
//  00:0 Continuous-conversion mode
//  01:1 Single-conversion mode
//  10:2 Sleep-mode. Device is placed in sleep-mode
//  11:3 Sleep-mode. Device is placed in sleep-mode
//
// Default value: 00000011  : Sleep mode
//
// The rest of bits must be set to zero

// Output readings in 16 bit 2's complement
#define MAG_OUT_X_H  0x03
#define MAG_OUT_X_L  0x04
#define MAG_OUT_Z_H  0x05
#define MAG_OUT_Z_L  0x06
#define MAG_OUT_Y_H  0x07
#define MAG_OUT_Y_L  0x08

// Arguments of busesFunction --------------------------
#define BUSES_GYR_READ      1    // Read Gyroscope (X) on top
#define BUSES_GYR_ZERO      2    // Set zero value to current rate
#define SPI_F_START         3    // Start SPI
#define SPI_F_END           4    // Ends SPI
#define SPI_F_EX8           5    // Exchange one bye
#define SPI_F_MODE          6    // Set SPI Mode
#define BUSES_ACC_READ      7    // Read Accelerometer (X) on top
#define BUSES_ACC_ZERO_XY   8    // Set zero value to current value in X and Y
#define BUSES_MAG_READ      9    // Read Magnetometer (X) on top
#define BUSES_I_RR         10    // I2C Read Register
#define BUSES_I_WR         11    // I2C Write Register
#define BUSES_I_SCAN       12    // I2C Scan Addr
#define BUSES_I_TRANS      13    // I2C Transfer
#define BUSES_GYR_SET_ZERO 14    // Manually set zero value to Gyroscope
#define BUSES_ACC_SET_ZERO 15    // Manually set zero value to Accelerometer
#define BUSES_MAG_SET_ZERO 16    // Manually set zero value to Magnetometer

// Arguments of internalRegistersFunction
#define INTREG_F_GRR        1    // Gyroscope read register
#define INTREG_F_FRR        2    // Accelerometer read register
#define INTREG_F_MRR        3    // Magnetometer read register
#define INTREG_F_GWR        4    // Gyroscope write register
#define INTREG_F_FWR        5    // Accelerometer write register
#define INTREG_F_MWR        6    // Magnetometer write register

// Public functions -----------------------------------
void busesInit(void);

// Command functions
int32_t busesFunction(ContextType *context,int32_t value);
int32_t spiNexangeFunction(ContextType *context,int32_t value);
int32_t spiSetSpeed(ContextType *context,int32_t value);
int32_t i2cSetSpeed(ContextType *context,int32_t value);
int32_t i2cTransfer(ContextType *context,int32_t value);
int32_t internalRegistersFunction(ContextType *context,int32_t value);

// Public test functions
#ifdef TEST_GYRO
 void gyrTest(void);
#endif
#ifdef TEST_ACCEL
 void accelTest(void);
#endif
#ifdef TEST_MAGNET
 void magnetTest(void);
#endif


#endif //_BUSES_MODULE

