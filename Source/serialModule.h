/*
 serial.h
 Serial functions header file
 */

#ifndef _SERIAL_MODULE
#define _SERIAL_MODULE

// Definitions ------------------------------------------------

#define BREAK "\r\n"                             // Line break
#define SERIAL1     (BaseSequentialStream*)&SD1  //Stream identier
#define SERIALBASE1 (BaseChannel *)&SD1          // Stream base channel cast
#define SERIAL2     (BaseSequentialStream*)&SD2  //Stream identier
#define SERIALBASE2 (BaseChannel *)&SD2          // Stream base channel cast

// Function prototypes ----------------------------------------

void serialStart(void);  // Starts serial channels #1 and #2

#ifdef TEST_SERIAL //---------------------------------------TEST_SERIAL START
void serialTest(void); // Test the serial module
#endif //-----------------------------------------------------TEST_SERIAL END

#endif //_SERIAL_MODULE

