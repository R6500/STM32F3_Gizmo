/*
 usbSerial.c
 Serial over USB functions
 */

#include "fp_config.h"   // MForth port main config
#include "gizmo.h"       // Gizmo basic definitions and includes
#include "usbSerial.h"   // Serial over USB module
#include "chprintf.h"    // chibios/RT printf function

#include "usbcfg.h" // USB configuration header file

/* Virtual serial port over USB.*/
SerialUSBDriver SDU1;

/*********************** PRIVATE FUNCTIONS ****************************/

#ifdef TEST_USB //---------------------------------------- TEST USB START

// Test function
// Provides echo on Serial over USB changin all chars to uppercase
// Used by testUSB function
static void usbUpcaseEcho(void)
 {
	uint32_t result;

	// Start message
	chprintf(StreamUSB,"\r\n---Upcase Echo Start---\r\n");

	// Never returns
	while (1)
	{
		result=getUSBchar();
		if (result!=Q_TIMEOUT)
		    {
			if ((result>='a')&&(result<='z'))
				result=result+('A'-'a');

			sendUSBchar(result);
			if (result==13) sendUSBchar(10);
		    };

	}
 }

#endif //------------------------------------------- TEST USB END

/*********************** PUBLIC FUNCTIONS *****************************/

void sendUSBstring(char *cad)
 // Sends a string
 {
 char car;

 //Verify that string is not empty
 if (cad[0]==0) return;

 do
   {
   car=*cad;  // Get one char
   if (car)   // If it is not NULL...
	   sendUSBchar(car);  // ...send it
   cad++;    // Go to nex char
   }
   while (car!=0);
 }

void sendUSBstringLN(char *cad)
// Sends a string and adds CR+LF
 {
 sendUSBstring(cad);
 sendUSBchar(13);
 sendUSBchar(10);
 }

// Starts a serial over USB connection
void startUSBserial(void)
 {
	/*
	 * Initializes a serial-over-USB CDC driver.
	 */
	sduObjectInit(&SDU1);
	sduStart(&SDU1, &serusbcfg);

	/*
	 * Activates the USB driver
	 */
	usbStart(serusbcfg.usbp, &usbcfg);
 }

#ifdef TEST_USB //---------------------------------------- TEST USB START

// Serial over USB entry point
void testUSB()
 {
 usbUpcaseEcho();
 }

#endif //------------------------------------------- TEST USB END


