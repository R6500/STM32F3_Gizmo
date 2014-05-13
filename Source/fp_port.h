/**************************************************
 *
 *  f p _ p o r t . h
 *
 *  PORT for STM32F3 Discovery
 *
 * General port include for the forth project
 *
 * PORT FILE : This file is implementation dependent
 *
 * This file provides includes for general definitions
 * that can be machine dependent.
 *
 * It also includes definitions for structures that are
 * also machine dependent
 *
 *************************************************/

#ifndef _FP_PORT_INCLUDE
#define _FP_PORT_INCLUDE

// Size definitions ----------------------------------------

#define WA_SIZE         400  // Working area size for each thread

// Port includes ------------------------------------------

#include "gizmo.h"         // Basic include for Gizmo
#include "chprintf.h"	   // chprintf function

// Port console definitions -------------------------------

// This define is duplicated in console.h
// Beware don't get out of sync!!
#define UNDEFINED_CONSOLE   0

// Definition to know if we have console
#define NO_CONSOLE (WhichConsole==UNDEFINED_CONSOLE)

// Definition for line break
// Default line break
//      BREAK_0    (CR+LF)
//      BREAK_1    (CR)
//      BREAK_2    (LF)
#define BREAK_DEFAULT BREAK_0

// Definition for printf function
#define consolePrintf(...)   if (WhichConsole!=0) chprintf(Console_BSS,  __VA_ARGS__ )

// Port element for program memory ------------------------

// This structure will be added to the program memory
// that will be loaded at start-up when the user dictionary
// is loaded.

typedef struct
 {
 uint32_t vref4096;              // Internal Vref*4096 (For STM32F3)
 }PortSave;

// Typical vref data
#define Vref_4096     4915200    // Vref(mv) * 4096 corresponds to 1.2V
                                 // From datasheet can be 1.16 < 1.2 < 125

// Port thread data information --------------------------

 typedef struct
     {
 	 // Working area from definition in
 	 // GCC/ARMCMx/chcore_v7m.h
 	 stkalign_t wa[THD_WA_SIZE(WA_SIZE) / sizeof(stkalign_t)];
 	 }portThreadInfo;

// Thread priority levels 
 
// Priority limits
#define MAX_PRIO    (HIGHPRIO-NORMALPRIO)
#define MIN_PRIO    (LOWPRIO-NORMALPRIO)

// Thread list protection
extern Mutex treadListMutex;
#define LOCK_TLIST	 chMtxLock(&treadListMutex);
#define UNLOCK_TLIST chMtxUnlock();

// PAD Definitions ---------------------------------------

// Address of the PAD (CCM Ram)
#define PAD_ADDRESS  (0x10000000)

// PAD size. Must be multiple of 4 (CCM Size)
#define PAD_SIZE     (8*1024)

// Flash memory definitions ------------------------------

// Flash memory positions
#define FLASH_START 0x08000000
#define FLASH_PAGE_SIZE  2048
#define FLASH_LAST_PAGE   127
#define FLASH_PAGE_START(npage)    (FLASH_START+(FLASH_PAGE_SIZE*(npage)))

// MACROS ------------------------------------------------

// The following macro should give a non zero value (true)
// if all current tasks should abort
// It should normaly be associated to a hardware line

// Associated with button press
// Used to cancel start word
// Used to abort al threads
#define PORT_ABORT  ((BUTTON_GPIO->IDR)&BIT(BUTTON_PIN))

// External console definitions in console.c
extern int32_t WhichConsole;                    // Console we are using
extern BaseSequentialStream *Console_BSS;       // Console base sequential stream
extern BaseChannel *Console_BC;                 // Console base channel

// Function prototypes -----------------------------------

// Console char function definitions
 int32_t consoleGetChar(void);
 void consolePutChar(int32_t value);

// Function, if any that will initialize the port section
void portSaveInit(PortSave *pointer);

// Functions to save and load the dictionary
int32_t saveUserDictionary(void);
int32_t loadUserDictionary(void);

// Thread function
int32_t portThreadCreate(int32_t nth, void *pointer);

// Port specific limits
void portShowLimits(void);

// Check of callbacks
int32_t isAnyCallback(void);

#endif //_FP_PORT_INCLUDE

