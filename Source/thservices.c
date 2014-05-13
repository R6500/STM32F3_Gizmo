/*
 thservices.c
 Thread services source file

 This module gives services to be used in threads
 it include semaphores and threads
 */

// Includes
#include "fp_config.h"     // MForth port main config
#include "fp_port.h"       // Foth port include
#include "fm_main.h"       // Forth Main header file
#include "fm_stack.h"      // Stack module header
#include "fm_screen.h"
#include "fm_debug.h"

#include "gizmo.h"         // Main include for the project
#include "thservices.h"    // This module header

// Semaphore array
BinarySemaphore semaphores[MAX_SEMAPHORES];

// Mutex array
Mutex mutexes[MAX_MUTEXES];

/***************** PUBLIC FUNCTIONS ***********************/

// Initializes semaphores and mutexes
void thservicesInit(void)
 {
 int32_t i;

 // Init thread protection mutex
 chMtxInit(&treadListMutex);

 // Initialize semaphores if any
 if (MAX_SEMAPHORES)
	 for(i=0;i<MAX_SEMAPHORES;i++)
		 chBSemInit(semaphores+i,FALSE);

 // Initialize mutexes if any
 if (MAX_MUTEXES)
	 for(i=0;i<MAX_MUTEXES;i++)
		       chMtxInit(mutexes+i);
 }

/***************** COMMAND FUNCTIONS **********************/

// Generic semaphore function
int32_t semaphoreFunction(ContextType *context,int32_t value)
 {
 int32_t num;

 // Get semaphore number if needed
 if ((value==SEM_F_SIGNAL)||(value==SEM_F_WAIT))
   {
   // Get the semaphore number
   if (PstackPop(context,&num)) return 0;

   // Check semaphore number
   if ((num<0)||(num>=MAX_SEMAPHORES))
        {
	    runtimeErrorMessage(context,"Invalid semaphore number");
	    return 0;
        }
   }

 switch (value)
     {
     case SEM_F_SIGNAL: // Signal the semaphore
    	 chBSemSignal(semaphores+num);
    	 break;

     case SEM_F_WAIT: // Wait the semaphore
    	 chBSemWait(semaphores+num);
    	 break;

     case SEM_F_LIST: // List all semaphores
    	 if (NO_RESPONSE(context)) return 0;
    	 consolePrintf("Semaphore list:%s",BREAK);
    	 for(num=0;num<MAX_SEMAPHORES;num++)
    	     {
    		 consolePrintf("  Sem %2d : ",num);
    		 if (chBSemGetStateI(semaphores+num))
    		 	 { consolePrintf("TAKEN%s",BREAK); }
    		    else
    		     { consolePrintf("FREE%s",BREAK); }
    		 }
    	 break;

     case SEM_F_RESET: // Reset all semaphores
    	 for(num=0;num<MAX_SEMAPHORES;num++)
    		       chBSemReset(semaphores+num,FALSE);
    	 break;

     default:
    	 DEBUG_MESSAGE("Cannot arrive to default in semaphoreFunction");
     }

 return 0;
 }


// Generic mutex function
int32_t mutexFunction(ContextType *context,int32_t value)
 {
 int32_t num=0;

 // Get mutex number if needed
 if (value==MTX_F_LOCK)
   {
   // Get the mutex number
   if (PstackPop(context,&num)) return 0;

   // Check mutex number
   if ((num<0)||(num>=MAX_MUTEXES))
        {
	    runtimeErrorMessage(context,"Invalid mutex number");
	    return 0;
        }
   }

 switch (value)
     {
     case MTX_F_LOCK: // Lock the mutex
    	 chMtxLock(mutexes+num);
    	 break;

     case MTX_F_UNLOCK: // Unlock last (owned) locked mutex
    	 chMtxUnlock();
    	 break;

     case MTX_F_UNLOCK_ALL: // Unlock all owned mutexes
    	 chMtxUnlockAll();
    	 break;

     default:
    	 DEBUG_MESSAGE("Cannot arrive to default in mutexFunction");
     }

 return 0;
 }

