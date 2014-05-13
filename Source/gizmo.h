/*
 * Main STM32F3 Gizmo header file
 */

#ifndef _GIZMO
#define _GIZMO

// Basic include files
#include "ch.h"          // ChibiOs OS
#include "hal.h"         // ChibiOS Hal
#include "base.h"        // Some basic definitions

// Version information
#define VERSION       "1.0"         // Version in text mode
#define VERSION_INT    100          // Version in integer mode (100*version)
#define VERSION_DATE  "25/4/2014"   // Date version

// DEBUG definitions

//#define USE_DEBUG    // If enabled, debug info will be send to console

// Test definitions
// Every definition enables the test functions of one module

//#define TEST_USB      // Enable test on Serial over USB Module
//#define TEST_SERIAL   // Enable test on Serial Module
//#define TEST_REGISTER // Enable test of the command Register
//#define TEST_ANALOG   // Enable test of the ADC
//#define TEST_GYRO     // Enable test of the Gyroscope
//#define TEST_ACCEL    // Enable test of the Accelerometer
//#define TEST_MAGNET   // Enable test of the Magnetometer

// LEDs definitions

#define LEDS_PORT      GPIOE   // LEDs in port E

// GPIO definitions

#define GPIO_PORT      GPIOD   // GPIOs in port D
#define GPIO0_PIN         2
#define GPIO1_PIN         3
#define GPIO2_PIN         4
#define GPIO3_PIN         7
#define GPIO4_PIN         9
#define GPIO5_PIN         10
#define GPIO6_PIN         12
#define GPIO7_PIN         13
#define GPIO8_PIN         14
#define GPIO9_PIN         15

// Button definitions

#define BUTTON_GPIO   GPIOA
#define BUTTON_PIN    0

// Serial port definitions

// Default serial speed
//#define SERIAL_SPEED 9600
#define SERIAL_SPEED 38400

#define SERIAL1_PORT    GPIOC   // Serial port for mapping
#define SERIAL1_TX_PIN  4       // Pin TX
#define SERIAL1_RX_PIN  5       // Pin RX

#define SERIAL2_PORT    GPIOD   // Serial port for mapping
#define SERIAL2_TX_PIN  5       // Pin TX
#define SERIAL2_RX_PIN  6       // Pin RX

// Analog channels definitions

#define NUM_CHANNELS    6       // Number of analog channels

#define ANALOG0_PORT    GPIOF
#define ANALOG0_PIN     4
#define ANALOG0_CHANNEL 5

#define ANALOG1_PORT    GPIOC
#define ANALOG1_PIN     0
#define ANALOG1_CHANNEL 6

#define ANALOG2_PORT    GPIOC
#define ANALOG2_PIN     1
#define ANALOG2_CHANNEL 7

#define ANALOG3_PORT    GPIOC
#define ANALOG3_PIN     2
#define ANALOG3_CHANNEL 8

#define ANALOG4_PORT    GPIOC
#define ANALOG4_PIN     3
#define ANALOG4_CHANNEL 9

#define ANALOG5_PORT    GPIOF
#define ANALOG5_PIN     2
#define ANALOG5_CHANNEL 10

#define ANALOG_NMEAN    100   // Default number of samples for mean

#define ANALOG_VREF_CHANNEL ADC_IN18

// DAC Definitions

#define DAC_PORT       GPIOA
#define DAC_PIN        4

// Gyroscope information

#define SPI1_PORT	    GPIOA
#define SPI1_SCK_PIN    5
#define SPI1_MISO_PIN   6
#define SPI1_MOSI_PIN   7
#define GYR_INT_PORT    GPIOE
#define GYR_INT1_PIN    0
#define GYR_INT2_PIN    1
#define GYR_CS_PORT     GPIOE
#define GYR_CS_PIN      3

// SPI3 Information

#define SPI3_PORT       GPIOC
#define SPI3_SCK_PIN    10
#define SPI3_MISO_PIN   11
#define SPI3_MOSI_PIN   12
#define SPI3_CS0_PIN     7
#define SPI3_CS1_PIN     8
#define SPI3_CS2_PIN     9
#define SPI3_CS3_PIN    13

// I2C2 Information

#define I2C2_SCL_PORT   GPIOF
#define I2C2_SCL_PIN    6
#define I2C2_SDA_PORT   GPIOA
#define I2C2_SDA_PIN    10

#endif //_GIZMO


/********************** BOARD AND PROCESSOR INFORMATION ****************************

 STM32F303VCT6

 Flash memory: 256 KBytes   (40000h)  Starts on: 0x08000000
 SRAM: 40 KBytes on Data Bus (A000h)  Starts on: 0x20000000
        8 KBytes CCM Memory  (2000h)  Starts on: 0x10000000 (Not used by linker)

 Stack size (Main and Process): 1 KB (400h)
 Total space occuped by basic stacks: 2KB (800h)

 RAM Memory Map: 0x20000000 : Start of memory
                 0x20000000 : Start of main stack
                 0x20000400 : End of main stack
                 0x20000400 : Start of main thread stack
                 0x20000800 : End of mainthread stack

 All this information is in the port ld file:
 M:\MCU\stm32_Juno\ChibiOS_2.6.2\os\ports\GCC\ARMCMx\STM32F3xx\ld\STM32F303xC.ld
 an in the ch.map file

 The processor receives its clock at PF0-OSC_IN at 8MHz
 This is the input to the PLL
 As is indicated in mcuconf.h the PLL multiplier is 9 so the main clock is 72 MHz
 This is the expected clock frequency for the STM32F303VCT6 MCU

 APB1 Preescaler (PPRE1) is 2 so APB1 frequency is 36 MHz
 APB2 Preescaler (PPRE2) is 2 so APB2 frequency is 36 MHz
 HCLK Preescaler (HPRE)  is 1 so AHB bus operates at 72 MHz

 Flash memory is 256kB organized in 128 pages of 2kB

 Data is stored with LSB first (Little Endian)

 **********************************************************************************/
