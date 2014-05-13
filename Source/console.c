/*
 console.h
 Console functions
 */

#include "fp_config.h"     // MForth port main config
#include "fp_port.h"
#include "gizmo.h"         // Gizmo basic definitions and includes
#include "usbSerial.h"     // Serial over USB module
#include "serialModule.h"  // Serial Module
#include "chprintf.h"	   // chprintf function
#include "fm_main.h"       // Main Forth header file
#include "fm_threads.h"    // Threads header file
#include "fm_stack.h"
#include "console.h"       // This module header file

// Variables that hold the stream to use after start
int32_t WhichConsole=UNDEFINED_CONSOLE;  // Console we are using
BaseSequentialStream *Console_BSS;       // Console base sequential stream
BaseChannel *Console_BC;                 // Console base channel

// Serial USB driver on usbSerial.c
extern SerialUSBDriver SDU1;

// Prototype definition
void consoleBreak(void);

/************** PRIVATE FUNCTIONS ********************************/

// Sends a string to the selected base channel
// Timeouts if it is not possible
static void channelSendStringTimeout(BaseChannel *ch,char *string)
 {
 int32_t result;

 // Do nothing if it is a null string
 if ((*string)==0) return;

 do
  {
  result=chnPutTimeout(ch,*string,10);   // Send one char
  string++;                              // Increase pointer
  }
  while(((*string)!=0)&&(result==Q_OK));
 }

// Waits until a character is received
// from the Serial #2 or Serial over USB
// Configure the console from the result
static void consoleWaitStart(void)
 {
 int received=0,result;

 channelSendStringTimeout((BaseChannel *)&SD2,"\r\nGizmo: Serial #2 waiting for input...\r\n");
 channelSendStringTimeout((BaseChannel *)&SDU1,"\r\nGizmo: Serial over USB waiting for input...\r\n");

 // Wait until a caracter is received
 while (!received)
 	{
 	// First try to read from serial channel #2
 	result=chnGetTimeout(SERIALBASE2,10);

 	// If something is received from the channel 2
 	if ((result==10)||(result==13))
 	    {
 		// Set console streams
 		Console_BSS=(BaseSequentialStream*)&SD2;
 		Console_BC=(BaseChannel *)&SD2;

 		// Set console information
 		WhichConsole=SERIAL2_CONSOLE;

 		// Ends detection
 		received=1;
 	    }

 	// First try to read from serial over USB
 	result=getUSBchar();

 	if ((result==10)||(result==13))
 	 	{
 	 	// Set console streams
 	 	Console_BSS=(BaseSequentialStream *)&SDU1;
 	 	Console_BC=(BaseChannel *)&SDU1;

 	 	// Set console information
 	 	WhichConsole=USB_CONSOLE;

 	 	// Ends detection
 	 	received=1;
 	 	}

 	}

 }

/*********************** PUBLIC FUNCTIONS ********************************/


// Runs several tests if enabled
static void consoleRunTests(void)
 {
 // Only for development
 // List all registered codes if it is enabled in conditional compilation
 #ifdef TEST_REGISTER
   registerList();
 #endif // TEST_REGISTER

 // Only for development
 // Test analog elements if it is enabled in conditional compilation
 #ifdef TEST_ANALOG
   analogTest();
 #endif //TEST_ANALOG

 // Only for development
 // Test Gyroscope if it is enabled in conditional compilation
 #ifdef TEST_GYRO
   gyrTest();
 #endif // TEST_GYRO

 // Only for development
 // Test Accelerometer if it is enabled in conditional compilation
 #ifdef TEST_ACCEL
   accelTest();
 #endif // TEST_ACCEL

 // Only for development
 // Test Accelerometer if it is enabled in conditional compilation
 #ifdef TEST_MAGNET
   magnetTest();
 #endif // TEST_MAGNET
 }

// Opens the console
// Never returns
void consoleStart(void)
 {
 // Wait for activity on Serial #2 or Serial over USB
 consoleWaitStart();

 //DEBUG_MESSAGE("Console starts receiving...");

 if ((MainContext.VerboseLevel)&VBIT_INFO)
   {
   chprintf(Console_BSS,"%sSTM32F3 Gizmo%s",BREAK,BREAK);
   chprintf(Console_BSS,"Version %s (%s)%s",VERSION,VERSION_DATE,BREAK);
   if (WhichConsole==SERIAL2_CONSOLE)
       chprintf(Console_BSS,"Ready to accept commands from Serial#2%s",BREAK);
   if (WhichConsole==USB_CONSOLE)
       chprintf(Console_BSS,"Ready to accept commands from USB%s",BREAK);
   //chprintf(Console_BSS,"%s",BREAK);
   }

 // Run tests if enabled
 consoleRunTests();
 }

/***************** COMMAND FUNCTIONS **********************/

int32_t consoleFunction(ContextType *context,int32_t value)
 {
 switch (value)
   {
   case CONSOLE_F_ANY:  // Implements CONSOLE?
	   switch (WhichConsole)
	      {
	      case UNDEFINED_CONSOLE:
	    	  PstackPush(context,0);
	    	  return 0;
	    	  break;

	      case SERIAL2_CONSOLE:
	    	  PstackPush(context,2);
	    	  return 0;
	    	  break;

	      case USB_CONSOLE:
	    	  PstackPush(context,10);
	    	  return 0;
	    	  break;
	      }
	   break;
   }
 return 0;
 }




