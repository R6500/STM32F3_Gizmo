/*
 serialModule.h
 Serial functions
 */

#include "fp_config.h"     // MForth port main config
#include "gizmo.h"         // Gizmo basic definitions and includes
#include "serialModule.h"  // This module header file
#include "chprintf.h"	   // chprintf function


// Serial port configurations
SerialConfig Serial1Config,Serial2Config;

/******************** STATIC FUNCTIONS *******************************/

#ifdef TEST_SERIAL //---------------------------------------TEST_SERIAL START

// Dummy serial TX test
static void serialTXtest(void)
 {
 while (1)
   {
   chprintf(SERIAL1,"Serial ch#1 test%s",BREAK);  // Show message on ch#1
   chprintf(SERIAL2,"Serial ch#2 test%s",BREAK);  // Show message on ch#2
   chThdSleep(500); // Wait half a second
   }
 }

// Serial #1 and #2 debug echo
// Echoes data received in #1 and #2 over channels #1 and #2
static void serialUpcaseEchoTest(void)
{
	int32_t result;
	while (1)
	{
		// First try to read from channel #1
		result=chnGetTimeout(SERIALBASE1,10);

		// If timeout...
		if (result==Q_TIMEOUT)
		    {
			// Try to rad from channel #2
			result=chnGetTimeout(SERIALBASE2,10);
		    }
		if (result!=Q_TIMEOUT)
		{
			// If it is lowercase...
			if ((result>='a')&&(result<='z'))
			     result=result+'A'-'a';  // Set to uppercase

			// Write the data to channels #1 and #2
			chnPutTimeout(SERIALBASE1,result,10);
			chnPutTimeout(SERIALBASE2,result,10);

			// If there is a line feed...
			if (result==13)
			    {
			    // Add a carry return in both channels
				chnPutTimeout(SERIALBASE1,10,10);
				chnPutTimeout(SERIALBASE2,10,10);
			    }
		};
	}
}

#endif //-----------------------------------------------------TEST_SERIAL END


/* Just some test code
// Checks if there is a high level on the serial 2 line
static int serial2check(void)
 {
 int result;

 // Configure serial RX pin as input with pull-down
 palSetPadMode(SERIAL1_PORT,SERIAL1_RX_PIN,PAL_MODE_INPUT_PULLDOWN);

 // Read if this pin is at high level
 }
 */

/******************** PUBLIC FUNCTIONS *******************************/

// Starts serial channels 1 and 2
void serialStart(void)
 {
	// Starts serial channel 1
	Serial1Config.speed=SERIAL_SPEED;              //Documentations says sc_speed!!
	sdStart((SerialDriver *)&SD1,&Serial1Config);  //Starts driver

	// Configure serial 1 port mappings
	palSetPadMode(SERIAL1_PORT,SERIAL1_TX_PIN, PAL_MODE_ALTERNATE(7));
	palSetPadMode(SERIAL1_PORT,SERIAL1_RX_PIN, PAL_MODE_ALTERNATE(7));

	// Starts serial channel 2
	Serial2Config.speed=SERIAL_SPEED;              //Documentations says sc_speed!!
	sdStart((SerialDriver *)&SD2,&Serial2Config);  //Starts driver

	// Configure serial 1 port mappings
	palSetPadMode(SERIAL2_PORT,SERIAL2_TX_PIN, PAL_MODE_ALTERNATE(7));
	palSetPadMode(SERIAL2_PORT,SERIAL2_RX_PIN, PAL_MODE_ALTERNATE(7));

    // If testing serial port, write a message

    #ifdef TEST_SERIAL //---------------------------------------TEST_SERIAL START
	// Message to serial
	chprintf(SERIAL1,"%sSerial 1 starts%s%s",BREAK,BREAK,BREAK);
	chprintf(SERIAL2,"%sSerial 2 starts%s%s",BREAK,BREAK,BREAK);
    #endif //-----------------------------------------------------TEST_SERIAL END
 }

#ifdef TEST_SERIAL //---------------------------------------TEST_SERIAL START

void serialTest(void) // Serial test entry point
 {
 //serialTXtest(); // Dummy serial TX test in CH#1 and CH#2
 serialUpcaseEchoTest(); // Upcase Echo test in CH#1 and CH#2
 }
#endif //-----------------------------------------------------TEST_SERIAL END

