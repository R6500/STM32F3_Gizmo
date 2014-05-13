/***********************************************************************
 *
 * m a i n . c
 *
 * PORT for STM32F3 Discovery
 *
 * Gizmo Forth main source file
 *
 * Version 1.0 (17/3/2014)
 *
 * PORT FILE : This file is implementation dependent
 *
 * This file just need to call the forth interpreter
 * calling:
 *
 *  forthInit function in fmain.c
 *  forthMain function in fmain.c
 *
 ***********************************************************************/

#include "fp_config.h"     // MForth port main config
#include "gizmo.h"         // Gizmo basic definitions and includes
#include "usbSerial.h"     // Serial over USB module
#include "serialModule.h"  // Serial module
#include "fm_main.h"       // Forth main module
#include "console.h"       // Console module
#include "analog.h"
#include "buses.h"         // Serial buses SPI/I2C
#include "thservices.h"
#include "timeModule.h"
#include "gpioModule.h"
#include "pwmModule.h"


// Main function ---------------------------------
int main(void)
 {
 // HAL Initialization
 halInit();

 // Kernel Initialization
 chSysInit();

 // Start GPIOs
 gpioModuleInit();

 // Start serial over USB
 startUSBserial();

 // Serial over USB test (never returns)
 //#ifdef TEST_USB
 //  testUSB();
 //#endif //TEST_USB

 // Init thread services
 thservicesInit();

 // Serial buses init
 busesInit();

 // Time module init
 timeInit();

 // Start serial
 serialStart();

 // Test serial module (never returns)
 //serialTest();

 // Initialize analog module
 analogInit();

 // Initialize the PWM module
 pwmModuleInit();

 // Load flash memory
 //flashLoad();



 // Remove echo from verbose level in windows command console
 // Don't need to be removed in ansicon console
 // This is a port characteristic
 //VerboseLevel&=(~VBIT_ECHO);

 // Show datatypes (testing only) -- Works Ok
 // Not for release version
 //ftestShowTypes();

 forthInit();

 // Start the console
 consoleStart();

 // Dump the start of memory;
 //ftestDumpMem();

 forthMain();

 // Infinite loop (We should never arrive here)
 while(1) { SLEEP_MS(1);};

 // It should never arrive here
 return 0;
 }



