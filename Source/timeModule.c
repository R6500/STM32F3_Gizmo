/*
 timeModule.c
 Time functions source file
 */

// Includes
#include "fp_config.h"     // MForth port main config
#include "fp_port.h"       // Foth port include
#include "fm_main.h"       // Forth Main header file
#include "fm_stack.h"      // Stack module header
#include "fm_program.h"
#include "fm_debug.h"
#include "fm_screen.h"
#include "fm_register.h"

#include "gizmo.h"         // Main include for the project
#include "serialModule.h"  // Serial module header
#include "timeModule.h"    // This module header

// GPT configurations for TIM6 and TIM7 ( Timers 1 and 2 )
GenTimer gpt[N_GPT];

/*********************** STATIC FUNCTIONS *****************************/

// Callbacks execute in their own interrupt context

// Callback for timer 1 (TIM6)
static void gptCallback1(GPTDriver *driver)
 {
 UNUSED(driver);
 if (gpt[0].Word==NO_WORD) return;
 clearInterruptContext();
 programExecute(&InterruptContext,gpt[0].Word,0);
 }

// Callback for timer 2 (TIM7)
static void gptCallback2(GPTDriver *driver)
 {
 UNUSED(driver);
 if (gpt[1].Word==NO_WORD) return;
 clearInterruptContext();
 programExecute(&InterruptContext,gpt[1].Word,0);
 }

// General Timer Initializations
static void initGPT(void)
 {
 // Configure GPT 1
 gpt[0].Config.frequency=TIM_DEF_FREQ;  // Default Freq
 gpt[0].Config.callback=gptCallback1;
 gpt[0].Driver=&GPTD6;        // Timer 6
 gpt[0].Word=NO_WORD;         // No word yet
 gpt[0].Status=TSTATUS_STOP;  // Not running

 // Configure GPT 2
 gpt[1].Config.frequency=TIM_DEF_FREQ;  // Default Freq
 gpt[1].Config.callback=gptCallback2;
 gpt[1].Driver=&GPTD7;    // Timer 7
 gpt[1].Word=NO_WORD;     // No word yet
 gpt[1].Status=TSTATUS_STOP;  // Not running
 }

// Try to get interval from the stack
// It must be greater than zero
// Returns 0 on error
static int32_t getInterval(ContextType *context,int32_t *num)
 {
 // Try to get from stack
 if (PstackPop(context,num)) return 0;

 // Check that it is positive
 if (((*num)<1)||((*num)>TIM_MAX_INTERVAL))
       {
	   consoleErrorMessage(context,"Invalid timer interval");
	   return 0;
       }

 return 1; // Ok
 }

// Try to get timer number from the stack
// Returns 0 on error
static int32_t getTimerNumber(ContextType *context,int32_t *num)
 {
 // Try to get from stack
 if (PstackPop(context,num)) return 0;

 // Check that it is valid
 if (((*num)<1)||((*num)>N_GPT))
       {
	   consoleErrorMessage(context,"Invalid timer number");
	   return 0;
       }

 return 1; // Ok
 }

// Try to get word number
// Returns 0 on error
static int32_t getWord(ContextType *context,int32_t *pos)
 {
 char *name;

 // Get word to dump
 name=tokenGet();

 // Check if name is noword
 if (!strCaseCmp(name,"NOWORD")) return NO_WORD;

 // Locate this user word
 (*pos)=locateUserWord(name);

 // Error if not found
 if ((*pos)==(int32_t)NO_WORD)
     {
	 consoleErrorMessage(context,"Word not found");
	 return 0;
     }

 return 1; // Ok
 }

/*********************** PUBLIC FUNCTIONS *****************************/

// This module initialization
void timeInit(void)
 {
 // Init global timers
 initGPT();
 }

// Try to get frequency from the stack
// It must be greater than zero
// Returns 0 on error
int32_t getFreq(ContextType *context,int32_t *freq)
 {
 // Try to get from stack
 if (PstackPop(context,freq)) return 0;

 // Check that it is positive
 if (((*freq)<TIM_MIN_FREQ)||((*freq)>TIM_MAX_FREQ))
       {
	   consoleErrorMessage(context,"Invalid frequency");
	   return 0;
       }

 return 1; // Ok
 }

// Check if there is any timer register callback
int32_t isAnyTimerCallback(void)
 {
 int32_t i,any=0;

 for(i=0;i<N_GPT;i++)
	 if (gpt[i].Word!=NO_WORD)
	     {
		 if (SHOW_INFO((&MainContext)))
			 { consolePrintf("Timer %d has a registered callback%s",i+1,BREAK); }
		 any=1;
	     }

 return any;
 }

/*********************** COMMAND FUNCTIONS ***************************/

// Basic generic time function
int32_t timeFunction(ContextType *context,int32_t value)
 {
 int32_t data,number,word=NO_WORD;

 switch (value)
     {
     case TIME_F_SLEEP: // Sleeps some time
    	 if (PstackPop(context,&data)) return 0;   // Pop one value

    	 if (data<=0) return 0;  // Check the value is positive

    	 chThdSleep(data);
    	 break;

     case TIME_F_TIMER_FREQ: // Sets timer ( freq n -- )
    	 // Try to get timer number
    	 if (!getTimerNumber(context,&data)) return 0;
    	 // Trt to get timer frequency
    	 if (!getFreq(context,&number)) return 0;
     	 // Set frequency
    	 gpt[data-1].Config.frequency=number;
    	 break;

     case TIME_F_TIMER_WORD: // Sets callback word
    	 // Try to get word
    	 if (!getWord(context,&word)) return 0;
    	 // Try to get timer number
    	 if (!getTimerNumber(context,&data)) return 0;
    	 // Set word
    	 gpt[data-1].Word=(uint16_t)word;
    	 break;

     case TIME_F_REPEAT: // Set repeat mode
    	 // Try to get timer number
    	 if (!getTimerNumber(context,&data)) return 0;
    	 // Try to get timer interval
    	 if (!getInterval(context,&number)) return 0;
    	 // Start if needed
    	 if (gpt[data-1].Status==TSTATUS_STOP)
    	     	   {
    		       gptStart(gpt[data-1].Driver,&gpt[data-1].Config);
    	     	   gpt[data-1].Status=TSTATUS_RUN;
    	     	   }
    	 // Start continuous mode
    	 gptStartContinuous(gpt[data-1].Driver,(gptcnt_t)number);
    	 break;

     case TIME_F_ONE: // Set One Shot
    	 // Try to get timer number
    	 if (!getTimerNumber(context,&data)) return 0;
    	 // Try to get timer interval
    	 if (!getInterval(context,&number)) return 0;
    	 // Start if needed
    	 if (gpt[data-1].Status==TSTATUS_STOP)
    	     	   {
    		       gptStart(gpt[data-1].Driver,&gpt[data-1].Config);
    	     	   gpt[data-1].Status=TSTATUS_RUN;
    	     	   }
       	 // Start one shot
    	 gptStartOneShot(gpt[data-1].Driver,(gptcnt_t)number);
    	 break;

     case TIME_F_PAUSE: // Pause the timer
    	 // Try to get timer number
    	 if (!getTimerNumber(context,&data)) return 0;
    	 // Pause the timer if running
    	 if (gpt[data-1].Status==TSTATUS_RUN)
    	              gptStopTimer(gpt[data-1].Driver);
    	 break;

     case TIME_F_DELAY: // Polled Delay
    	 // Try to get timer number
    	 if (!getTimerNumber(context,&data)) return 0;
    	 // Try to get timer interval
    	 if (!getInterval(context,&number)) return 0;
    	 //DISABLE;
    	 // Start if needed
    	 if (gpt[data-1].Status==TSTATUS_STOP)
    	  	   {
    	       gptStart(gpt[data-1].Driver,&gpt[data-1].Config);
    	   	   gpt[data-1].Status=TSTATUS_RUN;
    	   	   }
    	 // Stop the timer
    	 gptPolledDelay(gpt[data-1].Driver,(gptcnt_t)number);
    	 //ENABLE;
    	 break;

     case TIME_F_RESET: // Cancels timers and callbacks
    	 // Removes all callbacks
    	 for(data=0;data<N_GPT;data++)
    	 	 {
    		 // Remove callback
    		 gpt[data].Word=NO_WORD;

    		 // Pause the timer if running
    		 if (gpt[data].Status==TSTATUS_RUN)
    		           gptStopTimer(gpt[data].Driver);
    	 	 }

    	 break;

     default:
    	 DEBUG_MESSAGE("Cannot arrive to default in timeFunction");
     }

 return 0;
 }

/* TIMER INFORMATION

 APB1 Preescaler (PPRE1) is 2 so APB1 frequency is 36 MHz
 Timer input should be 36 MHzx2 = 72MHz
 De ello la minima frequencia del timer es 72MHz/65536=1098 Hz

 */
