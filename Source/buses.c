/*
 buses.c
 Serial SPI and I2C buses functions source file
 */


// Includes
#include "fp_config.h"     // MForth port main config
#include "fp_port.h"       // Foth port include
#include "fm_main.h"       // Forth Main header file
#include "fm_stack.h"      // Stack module header
#include "fm_debug.h"      // Debug module
#include "fm_screen.h"
#include "fm_program.h"    // Program module


#include "gizmo.h"         // Main include for the project
#include "chprintf.h"	   // chprintf function
#include "buses.h"         // This module header

// Gyroscope Variables --------------------------

// Zero rate values to compensate offset
static int32_t GyrZeroX=0;
static int32_t GyrZeroY=0;
static int32_t GyrZeroZ=0;

// Current gyro read values
static int32_t GyrX,GyrY,GyrZ;

// Accelerometer Variables ----------------------

// Zero X,Y,Z values to compensate offset
static int32_t AccZeroX=0;
static int32_t AccZeroY=0;
static int32_t AccZeroZ=0;

// Current gyro read values
static int32_t AccX,AccY,AccZ;

// Magnetometer Variables ----------------------

// Zero X,Y,Z values
static int32_t MagZeroX=0;
static int32_t MagZeroY=0;
static int32_t MagZeroZ=0;

// Current magnet read values
static int32_t MagX,MagY,MagZ;

// SPI Configuration variables -------------------

uint16_t SPI_Speed=4<<3;  // 32 divider gives 1.125MHz
uint16_t SPI_Mode=0;      // 0 00 Cpol=0 Cpha=0
                          // 1 01 Cpol=0 Cpha=1
                          // 2 10 Cpol=1 Cpha=0
                          // 3 11 Cpol=1 Cpha=1

int16_t SPI_Running=0; // SPI is not running now

// SPI Chip Selects pins at SPI3 port
const uint16_t SPI_ChipSelects[N_SPI_CS]={7,8,9,13};

// Information for SPI configuration ---------------
// The SPI configuration is defined in a typedef in
// ChibiOS_2.6.2\os\hal\platforms\STM32\SPIv2
/*
typedef struct {
  spicallback_t    end_cb;  // Funtion callback (NULL if not used)
  ioportid_t       ssport;  // The chip select line port.
  uint16_t         sspad;   // The chip select line pad number.
  uint16_t         cr1;     // SPI CR1 register initialization data.
  uint16_t         cr2;     // SPI CR2 register initialization data.
} SPIConfig;
*/

// Gyroscope data --------------------------


// SPI1 configuration for the gyroscope

// Data from L3GD20 Datasheet
// Max frequency is 10MHz
// Standby clock is high  CPOL=1
// SDI data is sampled in rising edges of the clock CPHA=1

// In SPI CR1 Register:
//   Bits 5..3 : BR2..0 define the speed
//   000: fPCLK/2
//   001: fPCLK/4
//   010: fPCLK/8
//   011: fPCLK/16
//   100: fPCLK/32
//   101: fPCLK/64
//   110: fPCLK/128
//   111: fPCLK/256
// As the peripheral bus for SPI1 is APB2 operates at 36MHz
// a :8 divider 010 gives a safe operation frequency of 4.5MHz
// The limit values available are:
//    000: 36MHz/2 = 18MHz
//    111: 36MHz/256 = 141kHz
static const SPIConfig spi1cfg = {
  NULL,             // No callback
  GYR_CS_PORT,      // CS Port
  GYR_CS_PIN,       // CS Pin
  // SPI CR1 Data
  SPI_CR1_BR_1 | SPI_CR1_CPOL | SPI_CR1_CPHA,
  // We don't need to set any value in CR2 to 8 bit operation
  0
};

// SPI Data ---------------------------------------------------

// Configuration for SPI3
static SPIConfig spi3cfg = {
  NULL,             // No callback
  SPI3_PORT,        // CS Port for SPI3
  SPI3_CS0_PIN,     // CS0 Pin
  // SPI CR1 Data
  4<<3,  // Default 1MHz with Cpol=Cpha=0
  // We don't need to set any value in CR2 to 8 bit operation
  0
};

// Accelerometer / Magnetometer data -(I2C1) ---------------------

/* Typedef used to define the I2C data

typedef struct {
  // TIMINGR register initialization.
  //Refer to the STM32 reference manual, the values are affected
  //by the system clock settings in mcuconf.h.
  uint32_t        timingr;
  // CR1 Register: Leave to zero unless you know what you are doing.
  uint32_t        cr1;  // CR1 Register
  // CR2 register initialization.
  // Only the ADD10 bit can eventually be specified here
  // in order to use 10bit addressing mode
  uint32_t        cr2;
} I2CConfig;
*/

// The crytical point is the TIMINGR
// The cr1 and cr2 feature default configurations in the low level driver

// Configuring the I2C timing is quite complex
//
// I2C1 and I2C2 are located in the APB1 bus that operates at 36MHz
// this is used to update internal registers
//
// The main I2C clock, however, comes from SYSCLOCK or HSI
// mcuconf.h sets the clock source for I2C1 and I2C2 to SYSCLOCK (72MHz)
//
// ST has an application to calculate all the timings
// http://www.st.com/web/en/catalog/tools/PF258335#
// We set 72MHz as I2C base clock
// Using this application we get:
//
// Standard mode  50kHz   TIMINGR=0x603028A1
// Standard mode 100kHz   TIMINGR=0x10C08DCF
// Fast mode     200kHz   TIMINGR=0xA010031A
// Fast mode     400kHz   TIMINGR=0x00E0257A
//
// All this data will be included in the constant array I2C_Speeds

// Configuration for I2C1
static I2CConfig i2c1cfg={
		0x10C08DCF,   // Normal mode at 100kHz
		0,            // Don't change CR1
		0             // Don't change CR2
        };

// I2C2 data -------------------------------------------------------

// Configuration for I2C1
static I2CConfig i2c2cfg={
		0x10C08DCF,   // Normal mode at 100kHz
		0,            // Don't change CR1
		0             // Don't change CR2
        };

//************************* Global variables **********************************/

// I2C Speeds configurations
I2C_Speed_Data I2C_Speeds[]={ {50,0x603028A1},  // Standard mode  50kHz
		                     {100,0x10C08DCF},  // Standard mode 100kHz
		                     {200,0xA010031A},  // Fast mode     200kHz
		                     {400,0x00E0257A}   // Fast mode     400kHz
                            };


/*********************** GYROSCOPE STATIC FUNCTIONS ***************************/

// Initializes the Gyroscope in SPI1
static void gyrInit(void)
 {
 uint8_t txBuf[2],rxBuf[2];

 // Initialize the SPI1 driver to access the Gyroscope
 // The ChibiOS board file already initializes the pins
 spiStart(&SPID1, &spi1cfg);

 // Activate the Gyro -------------------------------------------------------

 // Select data to transmit
 txBuf[0]=0x00;                         // Write, so read Flag not used
 //txBuf[0]|=0x40;                      // Autoincrement Flag is not needed
 txBuf[0]|=GYR_CREG1;                   // Register to write  GYR_CREG1
 txBuf[1]=BIT(3)|BIT(2)|BIT(1)|BIT(0);  // Value to write: PD|Xen|Yen|Zen

 // Select the slave
 spiSelect(&SPID1);

 // Exchange data
 spiExchange(&SPID1, 2, txBuf, rxBuf);

 // End of transmision
 spiUnselect(&SPID1);
 }

// Reads the Gyroscope
static void gyrRead(void)
 {
 uint8_t txBuf[8],rxBuf[8];
 int16_t *px,*py,*pz;

 // Set pointers
 px=(int16_t*)&(rxBuf[1]);
 py=px+1;
 pz=px+2;

 // Select data to transmit
 txBuf[0]=0x80;     // Read Flag
 txBuf[0]|=0x40;    // Autoincrement Flag is used
 txBuf[0]|=GYR_X_L; // Start at register GYR_X_L
 txBuf[1]=0xff;     // Dummy value
 txBuf[2]=0xff;     // Dummy value
 txBuf[3]=0xff;     // Dummy value
 txBuf[4]=0xff;     // Dummy value
 txBuf[5]=0xff;     // Dummy value
 txBuf[6]=0xff;     // Dummy value

 // Select the slave
 spiSelect(&SPID1);

 // Exchange data
 spiExchange(&SPID1, 7, txBuf, rxBuf);

 // End of transmision
 spiUnselect(&SPID1);

 // Set data
 GyrX=((int32_t)(*px))-GyrZeroX;
 GyrY=((int32_t)(*py))-GyrZeroY;
 GyrZ=((int32_t)(*pz))-GyrZeroZ;
 }

// Sets zero to current rates
static void gyrSetZero(void)
 {
 // Read Gyroscope
 gyrRead();

 // Set Zero to current rate
 GyrZeroX=GyrX+GyrZeroX;
 GyrZeroY=GyrY+GyrZeroY;
 GyrZeroZ=GyrZ+GyrZeroZ;
 }

/********************* SPI3 STATIC FUNCTIONS *********************************/

// Initializes the SPI
static void SPIInit(void)
 {
 uint32_t i;

 // Configure CS* pins
 for(i=0;i<N_SPI_CS;i++)
        {
	    // Configure to Output Push-Pull
        palSetPadMode(SPI3_PORT,SPI_ChipSelects[i],PAL_MODE_OUTPUT_PUSHPULL);
        // Set to high level
        SPI3_PORT->BSRR.H.set=BIT(SPI_ChipSelects[i]);
        }

 // Configure the pin mux for SPI3
 palSetPadMode(SPI3_PORT,SPI3_SCK_PIN, PAL_MODE_ALTERNATE(6));
 palSetPadMode(SPI3_PORT,SPI3_MISO_PIN, PAL_MODE_ALTERNATE(6));
 palSetPadMode(SPI3_PORT,SPI3_MOSI_PIN, PAL_MODE_ALTERNATE(6));
 }

// Starts SPI2 with slave n 0...3
// n is supossed to be verified before operating
static void SPIStart(int32_t n)
 {
 // Set ChipSelect to use
 spi3cfg.sspad=SPI_ChipSelects[n];

 // Initialize the SPI3 driver
 spiStart(&SPID3, &spi3cfg);

 // Select the slave
 spiSelect(&SPID3);

 // Set SPI  as running
 SPI_Running=1;
 }

// Ends SPI3 releasing slave
static void SPIEnd(void)
 {
 // End of transmision
 spiUnselect(&SPID3);

 // Set SPI  as not running
 SPI_Running=0;
 }

// SPI3 Exchange of one byte
static int32_t SPIExchangeByte(int32_t data)
 {
 uint8_t tx,rx;

 // Set data
 tx=(uint8_t)data;

 // Exchange data
 spiExchange(&SPID3,1,&tx,&rx);

 // Set result
 return (int32_t)rx;
 }

/************************ I2C STATIC FUNCTIONS *******************************/

// Initializes the I2C Channel 2 (I2CD2)
static void I2C2init(void)
 {
 // Configure the pin mux for I2C2
 palSetPadMode(I2C2_SCL_PORT,I2C2_SCL_PIN, PAL_MODE_ALTERNATE(4));
 palSetPadMode(I2C2_SDA_PORT,I2C2_SDA_PIN, PAL_MODE_ALTERNATE(4));
 }

// Write one register
// Returns 0 Correct operation
//         1 Operation error
//         2 Timeout
static int32_t I2C_WriteRegister(I2CDriver *driver,I2CConfig *config
		                         ,int32_t address,int32_t reg,int32_t value)
 {
 // TX and RX Buffers
 uint8_t txbuf[2];
 uint8_t rxbuf[2];
 // Message returned by i2c transmit function
 msg_t message;

 // Start I2C1
 i2cStart(driver,config);

 txbuf[0]=reg;   // Register to access
 txbuf[1]=value; // Value to write

 // Send command
 message=i2cMasterTransmitTimeout( driver,           // Driver
		                           address,          // Device Address
	                               txbuf,            // TX Buffer
	                               2,                // Bytes to send
	                               rxbuf,            // RX Buffer
	                               0,                // Bytes to receive
	                               1000);            // 10ms timeout

 // Stop the driver
 i2cStop(driver);

 if (message==RDY_RESET) return 1;   // Operation error

 if (message==RDY_TIMEOUT) return 2;  // Timeout error

 return 0; // No error
 }

// Read one register
// Returns 0 Correct operation
//         1 Operation error
//         2 Timeout
static int32_t I2C_ReadRegister(I2CDriver *driver,I2CConfig *config
		                       ,int32_t address,int32_t reg,int32_t *value)
 {
 // TX and RX Buffers
 uint8_t txbuf[2];
 uint8_t rxbuf[2];
 // Message returned by i2c transmit function
 msg_t message;

 // Start I2C1
 i2cStart(driver,config);

 txbuf[0]=reg;   // Register to access

 // Send command
 message=i2cMasterTransmitTimeout( driver,           // Driver
		                           address,          // Device Address
	                               txbuf,            // TX Buffer
	                               1,                // Bytes to send
	                               rxbuf,            // RX Buffer
	                               2,                // Bytes to receive
	                               1000);            // 10ms timeout

 // Stop the driver
 i2cStop(driver);

 if (message==RDY_RESET) return 1;   // Operation error

 if (message==RDY_TIMEOUT) return 2;  // Timeout error

 // Return value
 (*value)=rxbuf[0];

 return 0; // No error
 }

// Writes several Bytes and then reads several Bytes
// Returns 0 Correct operation
//         1 Operation error
//         2 Timeout
static int32_t I2C_WriteAndRead(I2CDriver *driver,I2CConfig *config,int32_t address
		                        ,uint8_t *tBuf,int32_t nt
		                        ,uint8_t *rBuf,int32_t nr)
 {
 // Message returned by i2c transmit function
 msg_t message;

 // Start I2C1
 i2cStart(driver,config);

 // Send command
 message=i2cMasterTransmitTimeout( driver,           // Driver
		                           address,          // Device Address
	                               tBuf,             // TX Buffer
	                               nt,               // Bytes to send
	                               rBuf,             // RX Buffer
	                               nr,               // Bytes to receive
	                               50);            // 10ms timeout

 // Stop the driver
 i2cStop(driver);

 if (message==RDY_RESET) return 1;   // Operation error

 if (message==RDY_TIMEOUT) return 2;  // Timeout error

 return 0; // No error
 }

// Check I2C address and register
// Returns 0 if they are ok
static int32_t I2C_Check(ContextType *context,int32_t addr,int32_t nreg)
 {
 // Check ADDR
 if ((addr<0x08)||(addr>0x77))
     {
	 consoleErrorMessage(context,"Invalid 7bit I2C address");
	 return 1;
     }

 // Check register
 if ((nreg<0)||(nreg>255))
     {
	 consoleErrorMessage(context,"Invalid 8bit I2C register");
     return 2;
     }

 return 0;
 }

/************ ACCELEROMETER / MAGNETOMETER STATIC FUNCTIONS ******************/

// Initializes the accelerometer
static void accelInit(void)
 {
 // Configure accelerometer CTRL_REG1
 // 100 Hz update rate
 // No low power enable
 // Activate all three axes
 I2C_WriteRegister(&I2CD1,&i2c1cfg,ACCEL_ADDR,ACCEL_CTRL_REG1_A,0x57);
 }

// Read accelerometer
// and places the result on AccX,AccY,AccZ
// Compensates zero error for X and Y
// Don't compensate Z axis
static void accelRead(void)
 {
 uint8_t tBuf[2];      // Transmit buffer
 uint8_t Buf[6];       // Receive buffer
 int16_t *px,*py,*pz;  // 16 bit integer values

 // Set pointers to match integers and bytes
 px=(int16_t*)&(Buf[0]);
 py=px+1;
 pz=px+2;

 // Set transmision buffer and repeat mode (0x80)
 tBuf[0]=ACCE_OUT_XL|0x80;

 // Read six registers XL,XH,YL,YH,ZL,ZH
 I2C_WriteAndRead(&I2CD1,&i2c1cfg,ACCEL_ADDR,tBuf,1,Buf,6);

 // Set results
 AccX=((int32_t)(*px))-AccZeroX;
 AccY=((int32_t)(*py))-AccZeroY;
 AccZ=((int32_t)(*pz))-AccZeroZ;
 }

// Sets zero to current rates of X and Y
static void accelSetZero(void)
 {
 // Read Accelerometer
 accelRead();

 // Set Zero to current rate for axes X and Y
 AccZeroX=AccX+AccZeroX;
 AccZeroY=AccY+AccZeroY;
 }

// Initializes the magnetometer
static void magnetInit(void)
 {
 // Configure magnetometer
 // 75 Hz data
 // +/-2.5 gauss range
 //        670 LSB/gauss X,Y
 //        600 LSB/gauss Z
 // Selects continuous conversion

 // Configure magnetometer MAG_CRA_REG_M to 75Hz data rate
 I2C_WriteRegister(&I2CD1,&i2c1cfg,MAGNET_ADDR,MAG_CRA_REG_M,6<<2);

 // Configure MAG_CRB_REG_M to medium gain
 // 011 +/2.5 gauss 670 LSB/gauss X,Y  600 LSB/gauss Z
 I2C_WriteRegister(&I2CD1,&i2c1cfg,MAGNET_ADDR,MAG_CRB_REG_M,3<<5);

 // Configure MAG_MR_REG_M to continuous conversion
 I2C_WriteRegister(&I2CD1,&i2c1cfg,MAGNET_ADDR,MAG_MR_REG_M,0);
 }

// Read magnetometer
// and places the result on MagX,MagY,MagZ
static void magnetRead(void)
 {
 int32_t value;
 uint8_t tBuf[2];
 uint8_t rBuf[6];
 volatile uint8_t Buf[6];
 volatile int16_t *px,*py,*pz;

 // Set pointers
 px=(int16_t*)&(Buf[0]);
 pz=px+1;
 py=px+2;

 // Set transmision buffer to first register
 tBuf[0]=MAG_OUT_X_H;

 // Read six registers
 I2C_WriteAndRead(&I2CD1,&i2c1cfg,MAGNET_ADDR,tBuf,1,rBuf,6);

 // Read also status register
 I2C_ReadRegister(&I2CD1,&i2c1cfg,MAGNET_ADDR,0x09,&value);

 // Reorder data for LSB and MSB
 Buf[0]=rBuf[1]; Buf[1]=rBuf[0];
 Buf[2]=rBuf[3]; Buf[3]=rBuf[2];
 Buf[4]=rBuf[5]; Buf[5]=rBuf[4];

 // Set results
 MagX=((int32_t)(*px))-MagZeroX;
 MagY=((int32_t)(*py))-MagZeroY;
 MagZ=((int32_t)(*pz))-MagZeroZ;
 }


/********************* GENERAL PUBLIC FUNCTIONS ******************************/

void busesInit(void)
 {
 // Initializes the gyroscope
 gyrInit();

 // Initializes the SPI
 SPIInit();

 // Initializes the Accelerometer
 accelInit();

 // Initializes the Magnetometer
 magnetInit();

 // Initializes I2C Channel 2
 I2C2init();
 }

/********************* COMMAND FUNCTIONS ************************************/

// Generic bus function
// Includes:
//   BUSES_GYR_READ      1    // Read Gyroscope (X) on top
//   BUSES_GYR_ZERO      2    // Set zero value to current rate
//   SPI_F_START         3    // Start SPI
//   SPI_F_END           4    // Ends SPI
//   SPI_F_EX8           5    // Exchange one bye
//   SPI_F_MODE          6    // Set SPI Mode
//   BUSES_ACC_READ      7    // Read Accelerometer (X) on top
//   BUSES_ACC_ZERO_XY   8    // Set zero value to current value in X and Y
//   BUSES_MAG_READ      9    // Read Magnetometer (X) on top
//   BUSES_I_RR         10    // I2C Read Register
//   BUSES_I_WR         11    // I2C Write Register
//   BUSES_I_SCAN       12    // I2C Scan Addr
//   BUSES_I_TRANS      13    // I2C Transfer
//   BUSES_GYR_SET_ZERO 14    // Manually set zero value to Gyroscope
//   BUSES_ACC_SET_ZERO 15    // Manually set zero value to Accelerometer
//   BUSES_MAG_SET_ZERO 16    // Manually set zero value to Magnetometer
int32_t busesFunction(ContextType *context,int32_t value)
 {
 int32_t data,data2;
 int32_t nreg,addr,result;
 uint8_t a8,b8;

 switch (value)
     {
     case BUSES_GYR_READ:    // Reads Gyroscope
    	 gyrRead();
    	 PstackPush(context,GyrZ);  // Push Z (will end a -2)
    	 PstackPush(context,GyrY);  // Push Y (will end a -1)
    	 PstackPush(context,GyrX);  // Push X (will end a  0)
         break;

     case BUSES_GYR_ZERO:    // Sets the Gyroscope read zero
    	 gyrSetZero();
         break;

     case BUSES_GYR_SET_ZERO:  // Manually set zero value to Gyroscope
    	 if (PstackGetSize(context)<3)
    	        {
    		    consoleErrorMessage(context,"Not enough data on the stack");
    		    return 0;
    	        }
    	 // Pop zero values
    	 PstackPop(context,&GyrZeroZ);
    	 PstackPop(context,&GyrZeroY);
    	 PstackPop(context,&GyrZeroX);
    	 break;

     case BUSES_ACC_READ:    // Reads Accelerometer
    	 accelRead();
    	 PstackPush(context,AccZ);  // Push Z (will end a -2)
    	 PstackPush(context,AccY);  // Push Y (will end a -1)
    	 PstackPush(context,AccX);  // Push X (will end a  0)
         break;

     case BUSES_ACC_ZERO_XY:    // Sets the Accel X,Y to zero
    	 accelSetZero();
         break;

     case BUSES_ACC_SET_ZERO:  // Manually set zero value to Accelerometer
    	 if (PstackGetSize(context)<3)
    	        {
    		 consoleErrorMessage(context,"Not enough data on the stack");
    		    return 0;
    	        }
    	 // Pop zero values
    	 PstackPop(context,&AccZeroZ);
    	 PstackPop(context,&AccZeroY);
    	 PstackPop(context,&AccZeroX);
    	 break;

     case BUSES_MAG_READ:    // Reads Magnetometer
    	 magnetRead();
    	 PstackPush(context,MagZ);  // Push Z (will end a -2)
    	 PstackPush(context,MagY);  // Push Y (will end a -1)
    	 PstackPush(context,MagX);  // Push X (will end a  0)
         break;

     case BUSES_MAG_SET_ZERO:  // Manually set zero value to Magnetometer
    	 if (PstackGetSize(context)<3)
    	        {
    		    consoleErrorMessage(context,"Not enough data on the stack");
    		    return 0;
    	        }
    	 // Pop zero values
    	 PstackPop(context,&MagZeroZ);
    	 PstackPop(context,&MagZeroY);
    	 PstackPop(context,&MagZeroX);
    	 break;

     case SPI_F_START:    // Starts on selected channel
    	 if (!PstackPop(context,&data)) // Try to pop one value
    		   if ((data>=0)&&(data<N_SPI_CS))  // Check number of slave
    			   SPIStart(data);
         break;

     case SPI_F_END:    // Ends on selected channel
    	 SPIEnd();
    	 break;

     case SPI_F_EX8:    // Exchange a Byte
    	 if (!PstackPop(context,&data)) // Try to pop one value
    		 if ((data>=0)&&(data<256))  // Check range
    		     {
    			 data2=SPIExchangeByte(data);
    			 PstackPush(context,data2);   // Push result
    		     }
    	 break;
     case SPI_F_MODE: // Set SPI Mode
    	 if (SPI_Running)
    	       {
    		   consoleErrorMessage(context,"Cannot change mode while active");
    	  	   return 0;
    	       }
    	 if (!PstackPop(context,&data)) // Try to pop one value
    	     {
    		 if ((data>=0)&&(data<4))  // Check range
    		    {
    			SPI_Mode=data;
    			spi3cfg.cr1=SPI_Speed|SPI_Mode;
    		    }
    		  else
    		    consoleErrorMessage(context,"Mode out of range");
    	     }
       	 break;

     case BUSES_I_RR: // I2C Read Register
    	 if (PstackGetSize(context)<2)  // Check stack size
    	     {
    		 consoleErrorMessage(context,"Not enough stack elements");
    		 return 0;
    	     }
    	 PstackPop(context,&nreg);                  // Pop number of register
    	 PstackPop(context,&addr);                  // Pop I2C addr
    	 if (I2C_Check(context,addr,nreg)) return 0;   // Check I2C addr
    	 result=I2C_ReadRegister(&I2CD2,&i2c2cfg,addr,nreg,&data);  // Read register
    	 if (result!=0)
    	     {
    		 data=-result;                               // Check errors
    		 consoleErrorInt(context,"I2C Error: ",data);
    	     }
		 PstackPush(context,data);                  // Push data
       	 break;

     case BUSES_I_WR: // I2C Write  Register
    	 if (PstackGetSize(context)<3)  // Check stack size
    	     {
    		 consoleErrorMessage(context,"Not enough stack elements");
    		 return 0;
    	     }
    	 PstackPop(context,&data);                  // Pop data to write on register
    	 PstackPop(context,&nreg);                  // Pop number of register
    	 PstackPop(context,&addr);                  // Pop I2C addr
    	 if ((data<0)||(data>255))     // Check data
    	     {
    		 consoleErrorMessage(context,"Invalid Byte to write");
    		 return 0;
    	     }
    	 if (I2C_Check(context,addr,nreg)) return 0;   // Check I2C addr
    	 result=I2C_WriteRegister(&I2CD2,&i2c2cfg,addr,nreg,data);   // Write register
    	 if (result!=0)              // Check errors
    	     {
    		 data=-result;
    		 consoleErrorInt(context,"I2C Error: ",data);
    	     }
		 break;

     case BUSES_I_SCAN: // I2C Scan Addr
    	 nreg=0; // Nothing found yet
         for(addr=0x08;addr<=0x77;addr++)
         	 {
        	 result=I2C_WriteAndRead(&I2CD2,&i2c2cfg,addr,&a8,0,&b8,0);  // Transfer nothing
           	 if (!result)  // Anything found
        	    {
        		PstackPush(context,addr);   // Push this addr
        		nreg++;
        		// Show info on console if we are in interactive mode
        		// and verbose allows it
        		if (SHOW_INFO(context))
        			 { consolePrintf("Addr Found: 0x%x%s",addr,BREAK); }
        	    }
         	 }
         PstackPush(context,nreg);  // Push number of addr found
         break;

     default:
          DEBUG_MESSAGE("Cannot arrive to default in busesFunction");
     }

 return 0;
 }

// Generic function to read and write
// to Gyrospope, Magnetometer and Accelerometer
// internal registers
// All registers are considered as unsigned bytes 0...255
int32_t internalRegistersFunction(ContextType *context,int32_t value)
 {
 int32_t data,nreg;

 uint8_t txBuf[2],rxBuf[2];

 // Check if it is a write register function
 if ((value==INTREG_F_GWR)||(value==INTREG_F_FWR)||(value==INTREG_F_MWR))
    {
	// Check stack size
	if (PstackGetSize(context)<2)
	      {
		  consoleErrorMessage(context,"Not enough data on stack");
	 	  return 0;
	      }

	// Pop data to write on register
	PstackPop(context,&data);

	// Check data range
	if ((data<0)||(data>255))
	          {
		      PstackPop(context,&nreg);  // Pop register number
		      consoleErrorMessage(context,"Invalid Byte value");
		 	  return 0;
		      }
    }

 // Pop register number
 PstackPop(context,&nreg);

 // Preliminar range check
 if ((nreg<0)||(nreg>255))
 	    {
	    consoleErrorMessage(context,"Invalid register number");
 		return 0;
 		}

 switch (value)
    {
    case INTREG_F_GRR:  // Read Gyroscope register
    	// Check register number
    	if ((nreg<0xF)
    		||((nreg>0x10)&&(nreg<=0x1F))
    		||(nreg>0x38))
    	    {
    		consoleErrorMessage(context,"Invalid register number");
    	 	return 0;
    	 	}

    	// Set tx buffer
    	txBuf[0]=nreg; txBuf[1]=0xFF;
    	txBuf[0]|=0x80;     // Read Flag in register number

    	// Select the slave
    	spiSelect(&SPID1);

    	// Exchange data
    	spiExchange(&SPID1,2,txBuf,rxBuf);

    	// End of transmision
    	spiUnselect(&SPID1);

    	// Push obtained result
    	data=rxBuf[1];
    	PstackPush(context,data);
    	break;

    case INTREG_F_GWR:  // Write Gyroscope register
    	// Check register number
    	if ((nreg<0x20)
    		||((nreg>0x26)&&(nreg<=0x2D))
            ||(nreg==0x2F)
            ||(nreg==0x31)
    		||(nreg>0x38))
            {
    		consoleErrorMessage(context,"Invalid register number");
    	 	return 0;
    	 	}

    	// Set tx buffer
    	txBuf[0]=nreg; txBuf[1]=data;

    	// Select the slave
    	spiSelect(&SPID1);

    	// Exchange data
    	spiExchange(&SPID1,2,txBuf,rxBuf);

    	// End of transmision
    	spiUnselect(&SPID1);
    	break;

    case INTREG_F_FRR:  // Read Accelerometer register
    	// Check register number to see if it is readable
    	if ((nreg<0x20)||(nreg>0x3D))
    	    {
    		consoleErrorMessage(context,"Invalid register number");
    	 	return 0;
    	 	}

    	// Read this register
    	I2C_ReadRegister(&I2CD1,&i2c1cfg,ACCEL_ADDR,nreg,&data);

    	// Push obtained result
    	PstackPush(context,data);
    	break;

    case INTREG_F_FWR:  // Write Accelerometer register
    	// Check register number to see if it is writtable
    	if ((nreg<0x20)
    		||((nreg>=0x27)&&(nreg<=0x2D))
    		||(nreg==0x2F)
    		||(nreg==0x31)
    		||(nreg==0x31)
    	    ||(nreg>0x3D))
    	    {
    		consoleErrorMessage(context,"Invalid register number");
    	 	return 0;
    	 	}

    	// Write this register
    	I2C_WriteRegister(&I2CD1,&i2c1cfg,ACCEL_ADDR,nreg,data);
    	break;

    case INTREG_F_MRR:  // Read Magnetometer register
    	// Check register number to see if it is readable
    	if ((nreg<0x00)
    		||((nreg>=0x0D)&&(nreg<=0x30))
    	    ||(nreg>0x32))
    	    {
    		consoleErrorMessage(context,"Invalid register number");
    	 	return 0;
    	 	}

    	// Read this register
    	I2C_ReadRegister(&I2CD1,&i2c1cfg,MAGNET_ADDR,nreg,&data);

    	// Push obtained result
    	PstackPush(context,data);
    	break;

    case INTREG_F_MWR:  // Write Magnetometer register
    	// Check register number to see if it is writtable
    	// Check register number to see if it is readable
    	if ((nreg<0x00)||(nreg>0x02))
    	        {
    		    consoleErrorMessage(context,"Invalid register number");
    	 	    return 0;
    	 	    }

    	// Write this register
    	I2C_WriteRegister(&I2CD1,&i2c1cfg,MAGNET_ADDR,nreg,data);
    	break;
    }

 return 0;
 }

/****************** SPI COMMAND FUNCTIONS *********************************/

// SPI n byte exange function
int32_t spiNexangeFunction(ContextType *context,int32_t value)
 {
 UNUSED(value);

 uint8_t tx[STACK_SIZE],rx[STACK_SIZE];   // Data arrays
 int32_t n,i,pos;

 if (PstackPop(context,&n)) return 0; // Try to pop number of bytes to transfer

 // Check if there are enough elements
 if ((n<1)||(n>PstackGetSize(context)))
      {
	  consoleErrorMessage(context,"Incorrect number of bytes");
 	  return 0;
      }

 // Fit all data on tx array
 for(i=0;i<n;i++)
      {
	  // Get stack position for this data
	  pos=((context->stack.Pointer)+STACK_SIZE-n+i+1)%STACK_SIZE;
	  // Fill tx array position
	  tx[i]=(context->stack.data)[pos];
      }

 // Exchange data
 spiExchange(&SPID3,n,&tx,&rx);

 // Fill stack with returned values
 for(i=0;i<n;i++)
      {
	  // Get stack position for this data
	  pos=((context->stack.Pointer)+STACK_SIZE-n+i+1)%STACK_SIZE;
	  // Fill tx array position
	  (context->stack.data)[pos]=rx[i];
      }

 return 0;
 }

// SPI set speed to operate
// Pops selected speed in kHz
// Range is from 150 to 16000
int32_t spiSetSpeed(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int32_t speed0,speed;
 int32_t i;

 if (SPI_Running)
       {
	   consoleErrorMessage(context,"Cannot change speed while active");
 	   return 0;
       }

 if (PstackPop(context,&speed0)) return 0; // Try to pop the speed

 // Check if it is out of range
 if (speed0<141)
       {
	   consoleErrorMessage(context,"SPI Speed too slow");
	   speed0=141;
	   }

 for(i=7;i>=0;i--)
      {
	  // Calculate speed for this code
	  speed=36000/(1<<(i+1));
	  // Check if we are over set speed
	  if (speed0<speed) {i++;  break;}
      }

 if (i<0) i=0;            // Check value
 speed=36000/(1<<(i+1));  // Calculate real speed

 // Set speed
 SPI_Speed=i<<3;
 spi3cfg.cr1=SPI_Speed|SPI_Mode;

 // Push the selected speed
 PstackPush(context,speed);

 return 0;
 }

/****************** I2C COMMAND FUNCTIONS *********************************/

// I2C set speed to operate
// Pops selected speed in kHz
// Range is from 50kHz to 400kHz
// although there are only 4 available frequecies
//  50kHz, 100kHz, 200kHz, 400kHz
int32_t i2cSetSpeed(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int32_t speed0,speed;
 int32_t i;

  if (PstackPop(context,&speed0)) return 0; // Try to pop the speed

 // Check if it is out of range
 if (speed0<50)
       {
	   consoleErrorMessage(context,"I2C Speed too slow");
	   // Set minimum speed
	   i2c2cfg.timingr=I2C_Speeds[0].TIMINGR;
	   // Push the minimum
	   speed=I2C_Speeds[0].speed;
	   PstackPush(context,speed);
	   return 0;
       }

 for(i=0;i<4;i++)
    {
	// If wr are over selected speed
	if (speed0<I2C_Speeds[i].speed) {i--; break;}
	}

	if (i>3) i=3;                  // Check value
    speed=I2C_Speeds[i].speed;     // Obtain real speed

    // Set speed
    i2c2cfg.timingr=I2C_Speeds[i].TIMINGR;

	// Push the selected speed
	PstackPush(context,speed);

	return 0;
 }

// I2C transfer  ("It")
// (addr)(dw1)..(dwn)(nw)(nr) -> (dnr)...(d1r)
int32_t i2cTransfer(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int32_t nr,nw,i,data,error,addr;
 uint8_t tBuf[STACK_SIZE],rBuf[STACK_SIZE];

 // Check if there is at least 3 data on the stack
 if (PstackGetSize(context)<3)
     {
	 consoleErrorMessage(context,"Not enough stack data");
	 return 0;
     }

 // Pop number of reads
 PstackPop(context,&nr);

 // Pop number of writes
 PstackPop(context,&nw);

 // Check nr range
 if ((nr<0)||(nr>STACK_SIZE))
     {
	 consoleErrorMessage(context,"Invalid number or reads");
	 return 0;
     }

 // Check nr range
 if ((nw<0)||(nw>(STACK_SIZE-3)))
     {
	 consoleErrorMessage(context,"Invalid number or writes");
	 return 0;
     }

 // Check stack size for enough data for all writes
 // Check if there is at least 3 data on the stack
 if (PstackGetSize(context)<(nw+1))
     {
	 consoleErrorMessage(context,"Not enough stack write data");
	 return 0;
     }

 // Pop all write data (if any)
 error=0;
 if (nw)
   for(i=0;i<nw;i++)
     {
	 PstackPop(context,&data);
	 if ((data<0)||(data>255))
	     {
		 error=1;
		 consoleErrorInt(context,"Invalid write data: ",data);
	     }
	    else
	     tBuf[nw-i-1]=data;  // We fill array in reverse order
     }
 if (error) return 0;

 // Pop address
 PstackPop(context,&addr);

 // Check addr
 // Check ADDR
 if ((addr<0x08)||(addr>0x77))
     {
	 consoleErrorMessage(context,"Invalid 7bit I2C address");
	 return 0;
     }

 // Do the transfer
 error=I2C_WriteAndRead(&I2CD2,&i2c2cfg,addr,tBuf,nw,rBuf,nr);

 // Check errors
 if (error)
    {
	PstackPush(context,-error);
	consoleErrorInt(context,"Transfer error: ",-error);
	return 0;
    }

 // Fill stack with results in reverse order
 if (nr)
   for(i=0;i<nr;i++)
       {
	   data=rBuf[nr-i-1];
	   PstackPush(context,data);
       }

 return 0;
 }



/********************** GYROSCOPE TEST FUNCTION ******************************/

#ifdef TEST_GYRO

// This test function test the Gyroscope
void gyrTest(void)
 {
 uint8_t txBuf[8],rxBuf[8];
 int16_t *px,*py,*pz;

 // Set pointers
 px=(int16_t*)&(rxBuf[1]);
 py=px+1;
 pz=px+2;

 // Activate the Gyroscope
 // Select data to transmit
 txBuf[0]=0x00;                         // Write so read Flag not used
 //txBuf[0]|=0x40;                      // Autoincrement Flag is not needed
 txBuf[0]|=GYR_CREG1;                   // Register to write  GYR_CREG1
 txBuf[1]=BIT(3)|BIT(2)|BIT(1)|BIT(0);  // Value to write

 // Select the slave
 spiSelect(&SPID1);

 // Exchange data
 spiExchange(&SPID1, 2, txBuf, rxBuf);

 // End of transmision
 spiUnselect(&SPID1);

 while(1) // Eternal loop
     {
	 // Break the loop if button is pushed
	 if ((BUTTON_GPIO->IDR)&BIT(BUTTON_PIN))
	 	      {
	 		  consoleErrorMessage(&MainContext,"Run aborted by user button");
	 		  return;
	 	      }


	 // Select data to transmit
	 txBuf[0]=0x80;     // Read Flag
	 txBuf[0]|=0x40;    // Autoincrement Flag is used
	 txBuf[0]|=GYR_X_L; // Start at register GYR_X_L
	 txBuf[1]=0xff;     // Dummy value
	 txBuf[2]=0xff;     // Dummy value
	 txBuf[3]=0xff;     // Dummy value
	 txBuf[4]=0xff;     // Dummy value
	 txBuf[5]=0xff;     // Dummy value
	 txBuf[6]=0xff;     // Dummy value

	 // Select the slave
     spiSelect(&SPID1);

     // Exchange data
	 spiExchange(&SPID1, 7, txBuf, rxBuf);

	 // End of transmision
	 spiUnselect(&SPID1);

	 // Show information
	 consolePrintf("x:%d\ty:%d\tz:%d%s",(*px),(*py),(*pz),BREAK);

	 // Sleep 1s
	 chThdSleep(1000);
	 }
 }

#endif // TEST_GYRO

/********************** ACCELEROMETER TEST FUNCTION ******************************/

#ifdef TEST_ACCEL

// This test function uses the accelerometer/magnetometer LSM303DLHC in accel mode
void accelTest(void)
 {
 int32_t result;
 uint8_t tBuf[4];
 uint8_t Buf[8];
 int16_t *px,*py,*pz;

 // Set pointers
 px=(int16_t*)&(Buf[0]);
 py=px+1;
 pz=px+2;

 consolePrintf("%sAccelerometer test%s%s",BREAK,BREAK,BREAK);

 // Configure accelerometer
 result=I2C_WriteRegister(&I2CD1,&i2c1cfg,ACCEL_ADDR,0x20,0x57);

 // Show result
 consolePrintf("After configure res: %d %s",result,BREAK);

 // Set transmision buffer and repeat mode (0x80)
 tBuf[0]=ACCE_OUT_XL|0x80;

 while(1) // Eternal loop
     {
	 // Break the loop if button is pushed
	 if ((BUTTON_GPIO->IDR)&BIT(BUTTON_PIN))
	 	      {
		      consolePrintf("Run aborted by user button");
	 		  return;
	 	      }

	 // Read six registers
	 I2C_WriteAndRead(&I2CD1,&i2c1cfg,ACCEL_ADDR,tBuf,1,Buf,6);

	 // Show information
	 consolePrintf("x:%d\ty:%d\tz:%d%s",(*px),(*py),(*pz),BREAK);

	 // Sleep 1s
	 chThdSleep(1000);
     }

 chprintf(Console_BSS,"%",BREAK);
 }

#endif // ACCEL_TEST

/********************** MAGNETOMETER TEST FUNCTION ******************************/

#ifdef TEST_MAGNET
// This test function uses the accelerometer/magnetometer LSM303DLHC in magnet mode
void magnetTest(void)
 {
 int32_t result,value;
 uint8_t tBuf[4];
 uint8_t rBuf[8];
 volatile uint8_t Buf[8];
 volatile int16_t *px,*py,*pz;

 // Set pointers
 px=(int16_t*)&(Buf[0]);
 pz=px+1;
 py=px+2;

 consolePrintf("%sMagnetometer test%s%s",BREAK,BREAK,BREAK);

 // Configure magnetometer MAG_CRA_REG_M to 15Hz data rate
 // Not needed as default value is 15Hz
 result=I2C_WriteRegister(&I2CD1,&i2c1cfg,MAGNET_ADDR,MAG_CRA_REG_M,4<<2);
 if (result<0)
      { consolePrintf("MAG_CRA_REG_M Fault code : ",result); }

 // Read of register
 I2C_ReadRegister(&I2CD1,&i2c1cfg,MAGNET_ADDR,MAG_CRA_REG_M,&result);
 DEBUG_INT("Register MAG_CRA_REG_M reads: ",result);

 // Configure MAG_CRB_REG_M to medium gain
 // 011 +/2.5 gauss 670 LSB/gauss X,Y  600 LSB/gauss Z
 result=I2C_WriteRegister(&I2CD1,&i2c1cfg,MAGNET_ADDR,MAG_CRB_REG_M,3<<5);
 if (result<0)
	 consoleErrorInt(&MainContext,"MAG_CRB_REG_M Fault code : ",result);

 // Read of register
 I2C_ReadRegister(&I2CD1,&i2c1cfg,MAGNET_ADDR,MAG_CRB_REG_M,&result);
 DEBUG_INT("Register MAG_CRB_REG_M reads: ",result);

 // Configure MAG_MR_REG_M to continuous conversion
 result=I2C_WriteRegister(&I2CD1,&i2c1cfg,MAGNET_ADDR,MAG_MR_REG_M,0);
 if (result<0)
	 consoleErrorInt(&MainContext,"MAG_MR_REG_M Fault code : ",result);

 // Read of register
 I2C_ReadRegister(&I2CD1,&i2c1cfg,MAGNET_ADDR,MAG_MR_REG_M,&result);
 DEBUG_INT("Register MAG_MR_REG_M reads: ",result);



 while(1) // Eternal loop
     {
	 // Break the loop if button is pushed
	 if ((BUTTON_GPIO->IDR)&BIT(BUTTON_PIN))
	 	      {
	 		  consoleErrorMessage(&MainContext,"Run aborted by user button");
	 		  return;
	 	      }

	 // Set transmision buffer and repeat mode (0x80)
	 tBuf[0]=MAG_OUT_X_H;//|0x80;

	 // Read six registers
	 result=I2C_WriteAndRead(&I2CD1,&i2c1cfg,MAGNET_ADDR,tBuf,1,rBuf,6);
	 if (result<0)
	 	 consoleErrorInt("Mag. read fault code : ",result);

	 I2C_ReadRegister(&I2CD1,&i2c1cfg,MAGNET_ADDR,0x09,&value);
	 /*
	 I2C_ReadRegister(&I2CD1,&i2c1cfg,MAGNET_ADDR,MAG_OUT_X_H,&value);
	 Buf[1]=value;
	 I2C_ReadRegister(&I2CD1,&i2c1cfg,MAGNET_ADDR,MAG_OUT_X_L,&value);
	 Buf[0]=value;
	 I2C_ReadRegister(&I2CD1,&i2c1cfg,MAGNET_ADDR,MAG_OUT_Z_H,&value);
	 Buf[3]=value;
	 I2C_ReadRegister(&I2CD1,&i2c1cfg,MAGNET_ADDR,MAG_OUT_Z_L,&value);
	 Buf[2]=value;
	 I2C_ReadRegister(&I2CD1,&i2c1cfg,MAGNET_ADDR,MAG_OUT_Y_H,&value);
	 Buf[5]=value;
	 I2C_ReadRegister(&I2CD1,&i2c1cfg,MAGNET_ADDR,MAG_OUT_Y_L,&value);
	 Buf[4]=value;
	 */

	 // Reorder data
	 Buf[0]=rBuf[1]; Buf[1]=rBuf[0];
	 Buf[2]=rBuf[3]; Buf[3]=rBuf[2];
	 Buf[4]=rBuf[5]; Buf[5]=rBuf[4];

	 // Show information
	 consolePrintf("x:%d\ty:%d\tz:%d%s",(*px),(*py),(*pz),BREAK);

	 // Sleep 1s
	 chThdSleep(1000);
     }

 consolePrintf("%",BREAK);
 }
#endif //TEST_MAGNET
