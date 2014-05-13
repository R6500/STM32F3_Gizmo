/**************************************************
 *
 *  f m _ t h r e a d s . c
 *
 *
 * Generic thread operations
 *
 * The functions included are port independent
 *
 *************************************************/

/*
    Threads implementation:
       Threads are system dependent.
       In order to encapsulate thread operation:
            This module calls the port function:
                  int32_t portThreadCreate(int32_t nth, void *pointer)
                  Where:
                     nth      is a thread number 1...MAX_THREADS (defined in fp_port.h)
                     pointer  is an opaque void pointer
                  If the port implementation cannot create the thread it should
                  return non zero. If it succeeds it should return 0.
                  If succeeds the new created thread should only call
                  the function:
                         void fThreadStart(void *pointer)

    Port crea thread y el thread llama
    la función threadStart(context)
    Si port no puede crear thread
    retorna no 0


 */

// Includes
#include "fp_config.h"         // Main configuration file
#include "fp_port.h"           // Main port definitions
#include "fm_main.h"           // Main forth header file
#include "fm_screen.h"         // Screen module
#include "fm_program.h"        // Program module
#include "fm_stack.h"          // Stack module
#include "fm_debug.h"          // Debug module
#include "fm_branch.h"         // Branch module
#include "fm_threads.h"        // This module header file

#ifdef USE_THREADS

// Forth thread array
FThreadData FThreads[MAX_THREADS];

// External variables
//extern uint16_t CodePosition;

/*********************** STATIC FUNCTIONS *********************/

// Executes a word as a new thread
// The parameter stack will be cloned from current context
// The return stack will be initialized to zero
//
//      Using, for instance, a previous word but its
//      information should be context dependent
// Parameter:
//    baseContext  is the father context
//    Position     is the new thread start position
//
// Returns  1..MAX_THREADS thread number if OK
//         -1 if no thread space is available
//         -2 if port cannot launch thread
//         -3 incorrect priority level
static int32_t threadExecuteWord(ContextType *baseContext
		                        ,int32_t position,int32_t priority)
 {
 int32_t i;

 // Check priority levels
 if ((priority<MIN_PRIO)||(priority>MAX_PRIO))
           {
	       runtimeErrorMessage(baseContext,"Invalid priority");
 	       return -3;
 	       }

 LOCK_TLIST  // Protect list from concurrent access
 // Find one free slot...
 for(i=0;i<MAX_THREADS;i++)
    if (FThreads[i].status==FTS_NONE)
	   {
       //...found
       //DEBUG_INT("Slot found: ",i+1);  //----------------------------
       // Set thread data
       FThreads[i].status=FTS_RUNNNING;
       FThreads[i].position=(uint16_t)position;
       FThreads[i].priority=priority;
       // Init context information
       FThreads[i].context.Counter=position;
       FThreads[i].context.Flags=0;
       FThreads[i].context.Process=i+1;
       FThreads[i].context.VerboseLevel=0;
       UNLOCK_TLIST  // Unprotect list from concurrent access
       // Clone parameter stack from current one
       PstackClone(baseContext,&(FThreads[i].context));
       // Init return stack to zero
       RstackInit(&(FThreads[i].context));

       //DEBUG_MESSAGE("Slot initialized"); //------------------------------------

       // Launch from port function that returns 0 if OK
       if (portThreadCreate(i+1,(void*)&(FThreads[i].context)))
           {
    	   // Change mode as it didn't succeed
    	   LOCK_TLIST  // Protect list from concurrent access
    	   FThreads[i].status=FTS_NONE;
    	   UNLOCK_TLIST  // Unprotect list from concurrent access
    	   runtimeErrorMessage(baseContext,"Port thread launch error");
    	   return -2;
           }

       // Return thread number if successful
       return i+1;
	   }

 UNLOCK_TLIST  // Unprotect list from concurrent access
 // Not found any free slot. Return error
 runtimeErrorMessage(baseContext,"No free thread slots");
 return -1;
 }

/*********************** PUBLIC FUNCTIONS *********************/

// Initializes all threads
// Called by forthInit
void threadsInit(void)
 {
 int32_t i;

 for(i=0;i<MAX_THREADS;i++)
   {
   FThreads[i].priority=0;
   FThreads[i].status=FTS_NONE;
   // Initialize port dependent info for thread i
   }
 }

// Start running one word from a new tread
// This function is called by the port function
// portThreadCreate
void fThreadStart(void *pointer)
 {
 programExecute((ContextType*)pointer
		        ,((ContextType*)pointer)->Counter,1);

 //DEBUG_INT("Unlocking thread : ",((ContextType*)pointer)->Process);

 // When the thread ends we set it as non working
 LOCK_TLIST    // Protect list from concurrent access
 FThreads[(((ContextType*)pointer)->Process)-1].status=FTS_NONE;
 UNLOCK_TLIST  // Unprotect list from concurrent access
 }

// Indicates if there is anything running in the background
int32_t anythingBackground(void)
 {
 int32_t i, any=0;

 for (i=0;i<MAX_THREADS;i++)
     {
	 if (FThreads[i].status!=FTS_NONE)
	 	 	 	 {
		         if (SHOW_INFO((&MainContext)))
		             {consolePrintf("Thread [%d] is active%s",i+1,BREAK);}
		 	 	 any=1;
	 	 	 	 }
     }

 return any;
 }

/*********************** COMMAND FUNCTIONS ************************/

// List the current process status
int32_t threadList(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int32_t i,found=0;

 // Check if verbose level allows
 if (NO_RESPONSE(context)) return 0;

 CBK;  // Line break

 LOCK_TLIST  // Protect list from concurrent access
 for(i=0;i<MAX_THREADS;i++)
	 if (FThreads[i].status!=FTS_NONE)
	   {
       found=1;
       consolePrintf("  %d : ",i+1);
       showWordName(FThreads[i].position);
       // Show priority
       consolePrintf(" Prio[%d] ",FThreads[i].priority);
       // Show status information
       switch (FThreads[i].status)
          {
          case FTS_RUNNNING:
        	  if (FThreads[i].context.Flags&CFLAG_ABORT)
        	       {consolePrintf(" Aborting%s",BREAK);}
        	      else
        	       {consolePrintf(" Running%s",BREAK); }
        	  break;
          }

	   }
 UNLOCK_TLIST  // Unprotect list from concurrent access

 if (!found) consolePrintf("No active threads%s",BREAK);


 return 0;
 }

// Command word to start a new thread
int32_t threadLaunch(ContextType *context,int32_t value)
 {
 char *name;
 uint16_t pos;
 int32_t res;
 int32_t prio=0;   // Default priority value

 // Get word to launch
 name=tokenGet();

 // Locate this user word
 pos=locateUserWord(name);

 // Error if not found
 if (pos==NO_WORD)
     {
	 consoleErrorMessage(context,"Word not found");
	 return 0;
     }

 // Interactive operation ------------------------

 if ((value==TL_F_INTERACTIVE)||(value==TL_F_INT_PRIO))
      {
	  // Get priority from stack in TL_F_INT_PRIO
	  if (value==TL_F_INT_PRIO)
	         if (PstackPop(context,&prio)) return 0;

      // Execute this word in a new thread
      res=threadExecuteWord(context,(int32_t) pos,prio);

      // Push value on the stack
      if (res<=0)
           PstackPush(context,0);    // Error
          else
           PstackPush(context,res);  // Correct operation
      }

 // Compilation operation ------------------------

 if ((value==TL_F_COMPILE)||(value==TL_F_COM_PRIO))
       {
	   // Try to compile the "THRD" or "THRD_PRIO" execute command
	   if (value==TL_F_COMPILE)
	       if (baseCode("THRD"))
	         {
	 	     consoleErrorMessage(context,"Error compiling THREAD");
	 	     return 0;
	         }
	   if (value==TL_F_COM_PRIO)
	   	    if (baseCode("THRD_PRIO"))
	   	     {
	   	 	 consoleErrorMessage(context,"Error compiling THPRIO");
	   	 	 return 0;
	   	     }

	   //DEBUG_INT("Allocated: ",(int32_t)pos);

	   // Allocate space and set word address
	   allocate16u(pos);
	   }

 return 0;
 }

// Execute one thread from a word
// The following 2 Bytes are uint16_t that points to the
// word that will start the new thread
int32_t threadExecuteFromWord(ContextType *context,int32_t value)
 {
 int32_t res,addr,prio=0;

 // Get address
 addr=getAddrFromHere(context);

 // Get priority if needed
 if (value==TEW_PRIORITY)
	 if (PstackPop(context,&prio)) return 0;

 // Execute this word in a new thread
 res=threadExecuteWord(context,addr,prio);

 // Push value on the stack
 if (res<=0)
        PstackPush(context,0);    // Error
       else
        PstackPush(context,res);  // Thread number

 return 0;
 }

// Cancel the process number obtained from
// the top of the stack
int32_t threadKill(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int32_t nth;

 // Try to get the thread number
 if (PstackPop(context,&nth)) return 0;

 // Check number range
 if ((nth<1)||(nth>MAX_THREADS))
       {
	   if (SHOW_ERROR(context))
		   runtimeErrorMessage(context,"Invalid thread number");
	   return 0;
       }

 // Check if this thread is active
 if (FThreads[nth-1].status==FTS_NONE)
       {
	   if (SHOW_ERROR(context))
		   consoleErrorMessage(context,"This thread is not running");
	   return 0;
       }

 // Set abort bit
 (FThreads[nth-1].context.Flags)|=CFLAG_ABORT;

 // Show info if enabled
 if (SHOW_INFO(context))
      consolePrintf("Thread [%d] set to abort%s",nth,BREAK);

 return 0;
 }

// Cancel all running background threads
int32_t threadKillAll(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int32_t i;

 // Search all threads
 for(i=0;i<MAX_THREADS;i++)
	 if (FThreads[i].status==FTS_RUNNNING)
	    {
		// Set abort bit
		(FThreads[i].context.Flags)|=CFLAG_ABORT;

		// Show info if enabled
		if (SHOW_INFO(context))
		      consolePrintf("Thread [%d] set to abort%s",i+1,BREAK);
	    }

 return 0;
 }

#endif //USE_THREADS

