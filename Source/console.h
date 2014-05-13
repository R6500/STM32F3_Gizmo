/*
 console.h
 Console functions header file
 */

#ifndef _CONSOLE_MODULE
#define _CONSOLE_MODULE

// Available consoles
// In WhichConsole variable
#define UNDEFINED_CONSOLE   0  // This define is duplicated in console.h
                               // Beware don't get out of sync!!
#define SERIAL2_CONSOLE     2
#define USB_CONSOLE        10

// Function prototypes
void consoleStart(void);

// Command functions
int32_t consoleFunction(ContextType *context,int32_t value);
#define CONSOLE_F_ANY   0

#endif //_CONSOLE_MODULE

