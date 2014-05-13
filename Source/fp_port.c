/**************************************************
 *
 *  f p _ p o r t . c
 *
 *   PORT for STM32F3 Discovery
 *
 * General port source file for the forth project
 *
 * PORT FILE : This file is implementation dependent
 *
 *
 *************************************************/

#include "fp_config.h"   // Main configuration file
#include "gizmo.h"       // Gizmo basic definitions and includes
#include "fp_port.h"     // This module header file
#include "fm_main.h"     // Main forth header file
#include "fm_program.h"  // Program header file
#include "fm_screen.h"   // Screen header file
#include "fm_threads.h"  // Threads header file
#include "fm_debug.h"
#include "timeModule.h"
#include "analog.h"

// Mutex to protect the thread list
Mutex treadListMutex;

// External definitions
extern UserDictionary  UDict;
extern uint32_t MainFlags;

/*************** THREAD INFORMATION ******************/

portThreadInfo ThreadData[MAX_THREADS];

// External definitions
extern FThreadData FThreads[MAX_THREADS];

/*************** PUBLIC FUNCTIONS ********************/

// Console character functions -------------------------

// Get one character from the console
// Block if there is none
int32_t consoleGetChar(void)
 {
 // Return 0 if we have no console
 if (WhichConsole==UNDEFINED_CONSOLE) return 0;

 return chnGetTimeout(Console_BC,TIME_INFINITE);
 }

// Put one character to the console
// Block if there if buffer is full
void consolePutChar(int32_t value)
 {
 // Return if there is no console
 if (WhichConsole==UNDEFINED_CONSOLE) return;

 chnPutTimeout(Console_BC,value,TIME_INFINITE);
 }

// Persistent data functions -----------------------------

// fport.h includes a definition for a PortSave typedef
// This will include data that will be saved with the
// rest of the forth program data
// portSaveInit initialized this structure
void portSaveInit(PortSave *pointer)
 {
 pointer->vref4096=Vref_4096;
 }

// Save and Load functions ------------------------------
// Defined in flashModule.c/h ---------------------------

// Functions to save all the data contained in the
// UserDictionary typedef contained in program.c

/********************** STATIC FUNCTIONS ****************************/



// Check if one flash page is empty
// That is, only reads 0xFFFFFFFF
// Returns 1 if it is empty (all "1"s)
//         0 if there is any "0"
static int32_t flashIsEmpty(int32_t page)
 {
 uint32_t i;
 uint32_t *pointer;

 // Set start of page
 pointer=(uint32_t*)FLASH_PAGE_START(page);

 // Check each position
 for(i=0;i<(FLASH_PAGE_SIZE/sizeof(int32_t));i++)
	 if ((*pointer)!=0xFFFFFFFF) return 0;

 // If we arrive here, the page is empty
 return 1;
 }

// Erases one of the flash pages
// Flash must be previously unlocked
// Returns 0 if it works ok
static int32_t flashErasePage(int32_t page)
 {
 // Wait for flash to not be busy
 while ((FLASH->SR)&(FLASH_SR_BSY));

 // Activate flash erase page
 (FLASH->CR)|=(FLASH_CR_PER);

 // Wait for flash to be ready --------------------------
 while ((FLASH->SR)&(FLASH_SR_BSY));

 // Indicate a direction within page
 (FLASH->AR)=FLASH_PAGE_START(page)+128;  // 128 offset just to be sure

 // Wait for flash to be ready --------------------------
 while ((FLASH->SR)&(FLASH_SR_BSY));

 // Start erase
 (FLASH->CR)|=(FLASH_CR_STRT);

 // Wait one cycle
 asm volatile("nop");

 // Check for flash not be busy
 while ((FLASH->SR)&(FLASH_SR_BSY));

 // Check EOP Flag
 if (!((FLASH->SR)&(FLASH_SR_EOP))) return 1;  // Error

 FLASH->CR &= ~FLASH_CR_PER;

 // Wait for flash to complete operation
 while (!((FLASH->SR)&(FLASH_SR_EOP)));

 // Clear EOP Flag
 (FLASH->SR)|=FLASH_SR_EOP;

 // clear bits
 FLASH->SR |= FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPERR;

 // Success
 return 0;
 }

/********************** PUBLIC FUNCTIONS ****************************/

// Loads programs from flash if they are present
// Debug messages probably won't show up on console
// as console is started afer calling this function.
// Returns 0 if OK
int32_t loadUserDictionary(void)
 {
 UserDictionary *pFlashMem;
 uint16_t *fpointer,*dpointer;
 uint32_t i;

 // Associates a pointer with start of flash data
 pFlashMem=(UserDictionary*)FLASH_PAGE_START(FLASH_LAST_PAGE-FLASH_PAGES+1);

 // Check if magic corresponds
 if ((pFlashMem->Base.magic)!=MAGIC_NUMBER)
     {
	 DEBUG_MESSAGE("No data on flash");
	 return 1;
     }

 // Load data from flash
 // Set pointers
 dpointer=(uint16_t*)&UDict;
 fpointer=(uint16_t*)FLASH_PAGE_START(FLASH_LAST_PAGE-FLASH_PAGES+1);
 // Program all positions
 for(i=0;i<(FLASH_PAGE_SIZE*FLASH_PAGES)/sizeof(uint16_t);i++)
     {
	 // Read halfword
	 (*dpointer)=(*fpointer);

	 // Increase pointers
	 dpointer++;
	 fpointer++;
     }

 MainFlags|=MFLAG_LOADED;
 DEBUG_MESSAGE("Flash loaded");

 // Analog recalibrate as flash could contain new
 // Vref calibration data
  analogCalibrateVdd();

 return 0;
 }


// Save current program memory to flash
int32_t saveUserDictionary(void)
 {
 //UserDictionary *header;
 volatile int32_t page;
 volatile int32_t i;
 volatile uint16_t *fpointer,*dpointer;
 volatile int32_t size;

 // Check if there are any program
 if (UDict.Base.lastWord==NO_WORD)
      {
      consoleErrorMessage(&MainContext,"There are no programs on memory");
	  return 0;
	  }

 // Check that the HSI is running
 if ((RCC->CR)&BIT(0))
	 { DEBUG_MESSAGE("HSI is enabled"); }
 if ((RCC->CR)&BIT(1))
	 { DEBUG_MESSAGE("HSI is ready"); }

 // Calcule size to write in (uint16 elements)
 //
 // Size in Bytes
 size=sizeof(UdictBase)+sizeof(PortSave)+UDict.Base.nextPos+8;
 //
 // Add one byte if odd
 if (size%2) size++;
 //
 // Check that the 8 overhead dont't get us out of flash
 if (size>(FLASH_PAGE_SIZE*FLASH_PAGES))
	      size=(FLASH_PAGE_SIZE*FLASH_PAGES);
 //
 // Divide to calculate number of uint16 elements
 size/=sizeof(uint16_t);

 DEBUG_INT("Number of uint16 elements to write: ",size);

 if ((MainContext.VerboseLevel)&VBIT_INFO)
 	 consolePrintf("%sUnlocking flash%s",BREAK,BREAK);

 // Check if flash is locked
 if ((FLASH->CR)&(FLASH_CR_LOCK))
    {
	// Try to unlock using keys
	(FLASH->KEYR)=FLASH_KEY1;
	(FLASH->KEYR)=FLASH_KEY2;

	// Check if the memory is unlocked
	if ((FLASH->CR)&(FLASH_CR_LOCK))
	    {
	    consoleErrorMessage(&MainContext,"Cannot unlock flash memory");
	    return 0;
	    }

	DEBUG_MESSAGE("Flash Unlocked");
    }

 if ((MainContext.VerboseLevel)&VBIT_INFO)
	 consolePrintf("Erasing flash%s",BREAK);

 // Erase all pages if needed
 for(i=0;i<FLASH_PAGES;i++)
    {
	// Page to erase
	page=FLASH_LAST_PAGE-i;
	if (!flashIsEmpty(page))
	    {
		if ((MainContext.VerboseLevel)&VBIT_DEBUG)
			 consolePrintf("Erasing page %d%s",page,BREAK);
        if (flashErasePage(page))
            {
    	    consoleErrorInt(&MainContext,"Cannot erase flash page ",page);
    	    return 0;
    	    }
	     }
	DEBUG_INT("Flash page erased: ",page);
	// Check that it is really erased
	if (!flashIsEmpty(page))
	             {
		         consoleErrorInt(&MainContext,"Unsuccesful erase of page ",page);
		    	 return 0;
	             }
	DEBUG_INT("Checked erase of page ",page);
    }

 if ((MainContext.VerboseLevel)&VBIT_INFO)
	 consolePrintf("Writing flash%s",BREAK);

 // Write to flash

 // Wait for flash to be prepared
 while ((FLASH->SR)&(FLASH_SR_BSY));

 // Set pointers
 dpointer=(uint16_t*)&UDict;
 fpointer=(uint16_t*)FLASH_PAGE_START(FLASH_LAST_PAGE-FLASH_PAGES+1);
 // Program all positions
 //for(i=0;i<(FLASH_PAGE_SIZE*FLASH_PAGES)/sizeof(uint16_t);i++)
 for(i=0;i<size;i++)
     {
	 // Wait for flash to be ready --------------------------
	 while ((FLASH->SR)&(FLASH_SR_BSY));

	 // Set programming mode
	 (FLASH->CR)|=FLASH_CR_PG;

	 // Wait for flash to be ready --------------------------
	 while ((FLASH->SR)&(FLASH_SR_BSY));

	 // Write halfword
	 (*fpointer)=(*dpointer);

	 // Wait two cycles
	 asm volatile("nop");
	 asm volatile("nop");

	 // Remove programming mode
	 FLASH->CR &= ~FLASH_CR_PG;

	 asm volatile("nop");

	 // Wait for flash to end
	 while ((FLASH->SR)&(FLASH_SR_BSY));

	 // Check operation
	 if ((*fpointer)!=(*dpointer))
	        {
	        consoleErrorInt(&MainContext,"Compare operation failed at uint ",i);
	 	    return 1;  // Error
	 	    }

	 // Check operation (EOP Flag)
	 if (!((FLASH->SR)&(FLASH_SR_EOP)))
	         {
	 	     consoleErrorInt(&MainContext,"Check operation failed at uint ",i);
	 	     return 1;  // Error
	         }

	 // Clear EOP Flag
	 (FLASH->SR)|=FLASH_SR_EOP;

	 // Wait for flash to be ready --------------------------
	 while ((FLASH->SR)&(FLASH_SR_BSY));

	 // Increase pointers
	 dpointer++;
	 fpointer++;
     }

 // clear bits
 FLASH->SR |= FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPERR;

 DEBUG_MESSAGE("Flash write end");

 // Lock flash
 (FLASH->CR)|=FLASH_CR_LOCK;
 DEBUG_MESSAGE("Flash is locked");

 if ((MainContext.VerboseLevel)&VBIT_INFO)
	 consolePrintf("%sFlash saving ended%s%s",BREAK,BREAK,BREAK);

 return 0;
 }

void portShowLimits(void)
 {
 consolePrintf("Limits for Gizmo:%s",BREAK);
 consolePrintf("  Number of semaphores: %d%s",MAX_SEMAPHORES,BREAK);
 consolePrintf("  Number of mutexes: %d%s",MAX_MUTEXES,BREAK);
 CBK;
 consolePrintf("  Timer min freq: %d%s",TIM_MIN_FREQ,BREAK);
 consolePrintf("  Timer max freq: %d%s",TIM_MAX_FREQ,BREAK);
 consolePrintf("  Timer max interval: %d%s",TIM_MAX_INTERVAL,BREAK);
 CBK;
 }

// Thread functions -----------------------------------------------------------
//-----------------------------------------------------------------------------

// Thread function
static msg_t threadFunction(void *arg)
 {
 fThreadStart(arg);

 // Release mutexes if any locked
 chMtxUnlockAll();

 return 0;
 }

// portThreadCreate
// Return 0 if OK
int32_t portThreadCreate(int32_t nth, void *pointer)
 {
 // Call the new thread
 chThdCreateStatic(ThreadData[nth-1].wa,sizeof(ThreadData[nth-1].wa)
			          ,NORMALPRIO+FThreads[nth-1].priority
			          ,threadFunction,pointer);
 return 0; // OK
 }

// Callback information ----------------------------------------------------------

// Gives non zero if there is any registered callback
int32_t isAnyCallback(void)
 {
 int32_t any=0;

 // Check if there is any callback in the timers
 if (isAnyTimerCallback()) any=1;

 return any;
 }


