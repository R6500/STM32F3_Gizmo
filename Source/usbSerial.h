/*
 usbSerial.h
 Serial over USB header file

 Functions to operate the Serial over USB connection
 */

#ifndef _USB_SERIAL
#define _USB_SERIAL

/************** DEFINITIONS **************************/

// USB Serial Stream
// To be used in functions like chprintf
// For instance: chprintf(StreamUSB,"Text: %d\r\n",number);
#define StreamUSB           (BaseSequentialStream *)&SDU1

/************** FUNCTION PROTORYPES *****************/

void startUSBserial(void);        // Start serial over USB connection
void sendUSBstring(char *cad);    // Sends a string
void sendUSBstringLN(char *cad);  // Sends a string and adds CR+LF

#ifdef TEST_USB //------------------------------------------ TEST USB START
void testUSB(void);               // Module test
#endif //--------------------------------------------------- TEST USB END


/************** MACRO FUNCTIONS *********************/

// Receive one character over USB
#define getUSBchar()   chnGetTimeout((BaseChannel *)&SDU1,10)

// Sends a characher over USB
#define sendUSBchar(car)    chnPutTimeout((BaseChannel *)&SDU1,car,10)

#endif // usbSerial.h



