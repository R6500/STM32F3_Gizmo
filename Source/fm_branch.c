/*******************************************************************
 *
 *  f m _ b r a n c h . c
 *
 * Branch functions for the Forth project
 *
 * This module implements conditionals and branches
 *
 ******************************************************************/

// Includes
#include "fp_config.h"     // Main configuration file
#include "fp_port.h"       // Include file for the port
#include "fm_main.h"       // Main header file
#include "fm_stack.h"      // Stack header file
#include "fm_register.h"   // Register header file
#include "fm_debug.h"      // Debug header file
#include "fm_screen.h"     // Screen header file
#include "fm_program.h"    // Program header file
#include "fm_branch.h"     // This module header file

// External variables
extern uint16_t CodePosition;

/***************** COMPILATION STATIC FUNCTIONS *****************/

// Write data to a previously allocated Udict position
static void set16uValue(int32_t position,uint16_t value)
 {
 uint16_t *pointer;

 pointer=(uint16_t*)&(UDict.Mem[position]);
 (*pointer)=value;
 }

// Initialize leave for a loop
// Pushes Rstack pointer on Pstack with BHEAD_LEAVE
static void initLeave(void)
 {
 int32_t data;

 // Get pointer from rstack
 // Add one because -1 cannot be coded
 data=(int32_t)(MainContext.rstack.Pointer)+1;

 // Add BHEAD_LEAVE
 data|=BHEAD_LEAVE;

 CPUSH(data); //Push on parameter stack
 }

// Process pendent leaves
static void processLeaves(void)
 {
 int32_t data,addr;
 uint16_t *pointer;

 // Pop value set by DO
 if (CPOP(&data))
     {
	 consoleErrorMessage(&MainContext,"Cannot process leaves");
	 return;
     }

 // Check that data is about leaves
 if ((data&HEAD_MASK)!=BHEAD_LEAVE)
      {
	  consoleErrorMessage(&MainContext,"Inconsistent leave processing");
	  return;
      }

 // Discard head
 data&=BRANCH_MASK;

 // Discard one because 1 was added
 data--;

 while(MainContext.rstack.Pointer>data)
     {
	 // Try to get one element from the return stack
	 if (RPOP(&addr))
	    {
		consoleErrorMessage(&MainContext,"Inconsistent leave processing");
		return;
	    }

	 // Check that data is about leaves
	 if ((addr&HEAD_MASK)!=BHEAD_LEAVE)
	       {
	 	   consoleErrorMessage(&MainContext,"Inconsistent leave processing");
	 	   return;
	       }

	 // Discard head
	 addr&=BRANCH_MASK;

	 // Set pointer
	 pointer=(uint16_t*)(UDict.Mem+addr);

	 // Set value in pointer to current code position
	 (*pointer)=CodePosition;
     }
  }

// Get the second element value in the return stack
// to be used in loop
// Returns 0 if OK
//         1 there is not enough data on return stack
static int32_t loopGetLimit(ContextType *context,int32_t *value)
 {
 int32_t pointer;

 // Get return stack pointer
 pointer=context->rstack.Pointer;

 // Check that there are at least two elements
 if (pointer<1) return 1;

 // Give the value
 (*value)=context->rstack.data[pointer-1];

 return 0;
 }

/***************** COMPILATION PUBLIC FUNCTIONS *****************/

// Code a uint16_t value at current position
// without any preceding code
// Returns 0 if it works OK
//         2 if there is no space to fit the number
int32_t allocate16u(uint16_t value)
 {
 uint16_t *pointer;

 // Check is there is space
 if ((CodePosition+1)>=UD_MEMSIZE)
     {
	 consoleErrorMessage(&MainContext,"Out of memory allocating uint16");
	 return 2;
     }

 // Set pointer
 pointer=(uint16_t*)(UDict.Mem+CodePosition);

 // Set value
 (*pointer)=value;

 // Increase counter
 CodePosition+=2;

 return 0;
 }

/***************** COMPILATION COMMAND FUNCTIONS *****************/

// IF ELSE THEN ---------------------------------------------------
/*
 *  IF : Code "JZ"
 *       CPUSH_HERE | BHEAD_IF
 *       allocate16u
 *
 *  ELSE : CPOP addr+head
 *         Code "JMP"
 *         CPUSH_HERE | BHEAD_IF
 *         allocate16u
 *         Check and remove BHEAD_IF from addr
 *         write addr with here
 *
 *  THEN : CPOP addr
 *         Check and remove BHEAD_IF from addr
 *         write addr with here
 */

int32_t CompileIF(ContextType *context,int32_t value)
 {
 UNUSED(value); UNUSED(context);

 // Try to compile the IF execute command
 if (baseCode("JZ"))
        {
	    consoleErrorMessage(&MainContext,"Error compiling IF");
	    return 0;
        }

 // Push current counter with IF head
 CPUSH_HERE(BHEAD_IF);

 // Allocate space for IF addr with a dummy value
 allocate16u(0);

 return 0;
 }

int32_t CompileELSE(ContextType *context,int32_t value)
 {
 UNUSED(value); UNUSED(context);
 int32_t data;

 // Pop value set by IF
 if (CPOP(&data))
     {
	 consoleErrorMessage(&MainContext,"ELSE not matched by IF");
	 return 0;
     }

 // Check that there is a related IF
 if ((data&HEAD_MASK)!=BHEAD_IF)
      {
	  consoleErrorMessage(&MainContext,"ELSE not matched by IF");
	  return 0;
      }

 // Discard head
 data&=BRANCH_MASK;

 // Try to compile the JMP execute command
 if (baseCode("JMP"))
        {
	    consoleErrorMessage(&MainContext,"Error compiling ELSE");
	    return 0;
        }

 // Push current counter with IF head
 CPUSH_HERE(BHEAD_IF);

 // Allocate space for JMP addr with a dummy value
 allocate16u(0);

 // Set IF addr to current position
 set16uValue(data,CodePosition);

 return 0;
 }

int32_t CompileENDIF(ContextType *context,int32_t value)
 {
 UNUSED(value); UNUSED(context);
 int32_t data;

 // Pop value set by IF or ELSE
 if (CPOP(&data))
     {
	 consoleErrorMessage(&MainContext,"THEN not matched by IF or ELSE");
	 abortCompile();
	 return 0;
     }

 // Check that there is a related IF or ELSE
 //if (!(data&BHEAD_IF))
 if ((data&HEAD_MASK)!=BHEAD_IF)
      {
	  consoleErrorMessage(&MainContext,"THEN not matched by IF or ELSE");
	  return 0;
      }

 // Discard head
 data&=BRANCH_MASK;

 // Set IF addr to current position
 set16uValue(data,CodePosition);

 return 0;
 }

// DO | +DO | -DO -- LOOP | @LOOP -- LEAVE | ?LEAVE ------------------

/*
 * DO : CODE "DO"
 *      Init_Leave   Pushes Rstack pointer on Pstack with BHEAD_LEAVE
 *      CPUSH_HERE(BHEAD_DO)
 *
 *  LOOP : CODE "LOOP"
 * +LOOP : CODE "+LOOP"
 *     After any of the two:
 *            CPOP addr+head
 *                Check and remove BHEAD_DO from addr
 *                allocate16u(addr)
 *     Process leaves
 *          CPOP value+Head
 *          Check and remove BHEAD_LEAVE from value
 *          If Rstack Pointer (RP) > value (RP0)
 *                       while (PL0<PL)
 *                             {
 *                             [PL0]=HERE;
 *                             PL0++;
 *                             }
 *      Code "UNLOOP" to restore Return stack
 *
 * LEAVE : CODE "JMP"
 *         RPUSH_HERE(BHEAD_LEAVE)
 *         allocate16u
 *
 * UNLOOP : Not a generator function but a normal one
 *          Pops two values from the return stack
 *
 */

int32_t CompileDO(ContextType *context,int32_t value)
 {
 UNUSED(value); UNUSED(context);
 // Init Leave
 initLeave();

 // Code for each kind of command
 switch (value)
   {
   case F_DO_NORMAL:
   // Try to compile the DO execute command
   if (baseCode("DO"))
	  {
      consoleErrorMessage(&MainContext,"Error compiling DO");
	  return 0;
	  }
   break;

   case F_DO_PLUS:
   // Try to compile the +DO execute command
   if (baseCode("+DO"))
	  {
      consoleErrorMessage(&MainContext,"Error compiling +DO");
	  return 0;
	  }
   // Push HERE address on return stack with BHEAD_LEAVE
   RPUSH_HERE(BHEAD_LEAVE);

   // Allocate space for JMP addr with 0 dummy value
   allocate16u(0);
   break;

   case F_DO_MINUS:
   // Try to compile the -DO execute command
   if (baseCode("-DO"))
	  {
      consoleErrorMessage(&MainContext,"Error compiling -DO");
	  return 0;
	  }
   // Push HERE address on return stack with BHEAD_LEAVE
   RPUSH_HERE(BHEAD_LEAVE);

   // Allocate space for JMP addr with 0 dummy value
   allocate16u(0);
   break;
   }

 // Push current position
 CPUSH_HERE(BHEAD_DO);

 return 0;
 }


// Compiles the words:
// LOOP @LOOP
int32_t CompileLOOP(ContextType *context,int32_t value)
 {
 UNUSED(value); UNUSED(context);
 int32_t data;

 if (value==F_LOOP)
	 if (baseCode("LOOP"))
	 	  {
	      consoleErrorMessage(&MainContext,"Error compiling LOOP");
	 	  return 0;
	 	  }

 if (value==F_NEW_LOOP)
	 if (baseCode("@LOOP"))
	 	  {
	      consoleErrorMessage(&MainContext,"Error compiling @LOOP");
	 	  return 0;
	 	  }

 // Pop value set by DO
 if (CPOP(&data))
     {
	 consoleErrorMessage(&MainContext,"LOOP or @LOOP not matched by DO");
	 return 0;
     }

 // Check that there is a related DO
 if ((data&HEAD_MASK)!=BHEAD_DO)
      {
	  consoleErrorMessage(&MainContext,"LOOP or @LOOP not matched by DO");
	  return 0;
      }

 // Discard head
 data&=BRANCH_MASK;

 // Allocate space for JMP addr with addr value
 allocate16u((uint16_t)data);

 // Process leaves
 processLeaves();

 if (baseCode("UNLOOP"))
 	{
 	consoleErrorMessage(&MainContext,"Error compiling UNLOOP");
 	return 0;
 	}

 return 0;
 }

int32_t CompileLEAVE(ContextType *context,int32_t value)
 {
 UNUSED(value); UNUSED(context);
 // Try to compile the JMP execute command
 if (baseCode("JMP"))
        {
	    consoleErrorMessage(&MainContext,"Error compiling LEAVE");
	    return 0;
        }

 // Push HERE address on return stack with BHEAD_LEAVE
 RPUSH_HERE(BHEAD_LEAVE);

 // Allocate space for JMP addr with 0 dummy value
 allocate16u(0);

 return 0;
 }

// Compiles the ?LEAVE Word
int32_t CompileQ_LEAVE(ContextType *context,int32_t value)
 {
 UNUSED(value); UNUSED(context);
 // Try to compile the JMP execute command
 if (baseCode("JNZ"))
        {
	    consoleErrorMessage(&MainContext,"Error compiling ?LEAVE");
	    return 0;
        }

 // Push HERE address on return stack with BHEAD_LEAVE
 RPUSH_HERE(BHEAD_LEAVE);

 // Allocate space for JNZ addr with 0 dummy value
 allocate16u(0);

 return 0;
 }

// BEGIN -- UNTIL -- LEAVE | ?LEAVE ------------------

/*
 * BEGIN : Init_Leave   Pushes Rstack pointer on Pstack with BHEAD_LEAVE
 *         CPUSH_HERE(BHEAD_BEGIN)
 *
 * UNTIL : CPOP addr+HEAD
 *         Check and remove BHEAD_BEGIN
 *         CODE "JZ"
 *         allocate16u(addr)
 *         Process leaves
 */

int32_t CompileBEGIN(ContextType *context,int32_t value)
 {
 UNUSED(value); UNUSED(context);

 // Init Leave
 initLeave();

 // Push current position
 CPUSH_HERE(BHEAD_BEGIN);

 return 0;
 }

int32_t CompileUNTIL(ContextType *context,int32_t value)
 {
 UNUSED(value); UNUSED(context);

 int32_t addr;

 // Pop value set by BEGIN
 if (CPOP(&addr))
	    {
		consoleErrorMessage(&MainContext,"UNTIL not matched by BEGIN");
		return 0;
	    }

 // Check that there is a related BEGIN
 if ((addr&HEAD_MASK)!=BHEAD_BEGIN)
      {
	  consoleErrorMessage(&MainContext,"UNTIL not matched by BEGIN");
	  return 0;
      }

 // Discard head
 addr&=BRANCH_MASK;

 // Try to compile the JZ execute command
 if (baseCode("JZ"))
        {
	    consoleErrorMessage(&MainContext,"Error compiling UNTIL");
	    return 0;
        }

 // Allocate space for JZ addr with addr value
 allocate16u((uint16_t)addr);

 // Process leaves
 processLeaves();

 return 0;
 }

// BEGIN -- WHILE -- REPEAT/AGAIN -- LEAVE | ?LEAVE ------------------

/*
 * WHILE : CODE "JZ"
 *         RPUSH HERE|BHEAD_LEAVE
 *         allocate16u dummy
 *
 * REPEAT : CPOP addr+HEAD
 *          Check and remove BHEAD_BEGIN
 *          CODE "JMP"
 *          allocate16u(addr)
 *          Process leaves
 */

int32_t CompileWHILE(ContextType *context,int32_t value)
 {
 UNUSED(value); UNUSED(context);

 // Try to compile the JZ execute command
 if (baseCode("JZ"))
        {
	    consoleErrorMessage(&MainContext,"Error compiling WHILE");
	    return 0;
        }

 // Push HERE address on return stack with BHEAD_LEAVE
 RPUSH_HERE(BHEAD_LEAVE);

 // Allocate space for JNZ addr with 0 dummy value
 allocate16u(0);

 return 0;
 }

// Implements REPEAT and AGAIN
int32_t CompileREPEAT(ContextType *context,int32_t value)
 {
 UNUSED(value); UNUSED(context);

 int32_t addr;

 // Pop value set by BEGIN
 if (CPOP(&addr))
	    {
		consoleErrorMessage(&MainContext,"REPEAT/AGAIN not matched by BEGIN");
		return 0;
	    }

 // Check that there is a related BEGIN
 if ((addr&HEAD_MASK)!=BHEAD_BEGIN)
      {
	  consoleErrorMessage(&MainContext,"REPEAT/AGAIN not matched by BEGIN");
	  return 0;
      }

 // Discard head
 addr&=BRANCH_MASK;

 // Try to compile the JMP execute command
 if (baseCode("JMP"))
        {
	    consoleErrorMessage(&MainContext,"Error compiling REPEAT/AGAIN");
	    return 0;
        }

 // Allocate space for JMP addr with addr value
 allocate16u((uint16_t)addr);

 // Process leaves
 processLeaves();

 return 0;
 }

// CASE OF ENDOF ENDCASE ------------------
/*
 * CASE ( vcmp -- vcmp )
 *  <n1> OF commands... ENDOF
 *  <n2> OF commnads... ENDOF
 *  default...
 *  ENDCASE
 *
 * CASE : Init Leave
 *        // CPUSH BHEAD_CASE  // Not for now
 *
 * OF : CODE "OF"
 *      CPUSH HERE|BHEAD_OF
 *      allocate16 (dummy 0)
 *
 *
 * ENDOF : CPOP addr
 *         Check its head is BHEAD_OF
 *         Remove HEAD
 *         CompileLEAVE
 *         [addr]=HERE  // From previous OF allocate16
 *
 * ENDCASE : //CPOP addr
 *           //Check its head is BHEAD_OF
 *           //Remove HEAD
 *           //[addr]=HERE  // From previous OF allocate16
 *
 *           CODE "DROP"
 *
 *           Process Leaves
 *
 *
 *
 *
 *
 * Compiling words: CASE OF ENDOF ENDCASE
 *
 * Executing words: OF : Get addr from code
 *                       Pop value from stack
 *                       Check against current top
 *                       If not equal jump to addr
 */

int32_t CompileCASE(ContextType *context,int32_t value)
 {
 UNUSED(value); UNUSED(context);

 // Init Leave
 initLeave();

 // Push BHEAD_CASE
 //if (CPUSH(BHEAD_CASE))
 //    {
 //	 consoleErrorMessage("Error compiling CASE");
 //	 return 0;
 //    }

 return 0;
 }

int32_t CompileOF(ContextType *context,int32_t value)
 {
 UNUSED(value); UNUSED(context);

 // Try to compile the OF execute command
 if (baseCode("OF"))
        {
	    consoleErrorMessage(&MainContext,"Error compiling WHILE");
	    return 0;
        }

 // Push current address to be used by endof
 CPUSH_HERE(BHEAD_OF);

 // Allocate space for addr with 0 dummy value
 allocate16u(0);

 return 0;
 }

int32_t CompileENDOF(ContextType *context,int32_t value)
 {
 UNUSED(value);
 int32_t addr;

 // Pop value set by OF
 if (CPOP(&addr))
	    {
		consoleErrorMessage(&MainContext,"ENDOF not matched by OF");
		return 0;
	    }

 // Check that there is a related OF
 if ((addr&HEAD_MASK)!=BHEAD_OF)
      {
	  consoleErrorMessage(&MainContext,"ENDOF not matched by OF");
	  return 0;
      }

 // Discard head
 addr&=BRANCH_MASK;

 // Compile leave to go to the endcase
 CompileLEAVE(context,0);

 // Set OF addr to current position
 set16uValue(addr,CodePosition);

 return 0;
 }

int32_t CompileENDCASE(ContextType *context,int32_t value)
 {
 UNUSED(value); UNUSED(context);

 // Try to compile the DROP
 if (baseCode("DROP"))
        {
 	    consoleErrorMessage(&MainContext,"Error compiling ENDCASE");
 	    return 0;
        }

 // Process leaves
 processLeaves();

 return 0;
 }

/***************** EXECUTION PUBLIC FUNCTIONS ********************/

// Get addr from current counter position
int32_t getAddrFromHere(ContextType *context)
 {
 uint16_t *pointer;
 int32_t  value;

 // Pointer to current counter position
 pointer=(uint16_t*)&(UDict.Mem[context->Counter]);

 // Increase counter
 (context->Counter)+=2;

 // Return value
 value=(int32_t)(*pointer);
 return value;
 }

/***************** EXECUTION COMMAND FUNCTIONS *******************/

// Unconditional jump    [HIDDEN]
int32_t Jump(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int32_t addr;

 // Get address
 addr=getAddrFromHere(context);

 // Jump
 (context->Counter)=addr;

 return 0;
 }

// Pops a value from the stack and jumps if it is zero
// [HIDDEN]
int32_t JumpIfZero(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int32_t addr,data;

 // Get address
 addr=getAddrFromHere(context);

 // Pop value from stack
 if (PstackPop(context,&data))
       {
	   runtimeErrorMessage(context,"Stack underflow in JMP");
	   return 0;
       }

 // Jump if needed
 if (!data)  (context->Counter)=addr;

 return 0;
 }

// Pops a value from the stack and jumps if it is not zero
// [HIDDEN]
int32_t JumpIfNotZero(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int32_t addr,data;

 // Get address
 addr=getAddrFromHere(context);

 // Pop value from stack
 if (PstackPop(context,&data))
       {
	   runtimeErrorMessage(context,"Stack underflow in JMP");
	   return 0;
       }

 // Jump if needed
 if (data)  (context->Counter)=addr;

 return 0;
 }


// DO +DO -DO execute words    [HIDDEN]
int32_t ExecuteDO(ContextType *context,int32_t value)
 {
 int32_t limit,index,addr;

 if (PstackPop(context,&index))
      {
	  runtimeErrorMessage(context,"Not enough DO parameters");
	  return 0;
      }

 if (PstackPop(context,&limit))
      {
	  runtimeErrorMessage(context,"Not enough DO parameters");
 	  return 0;
      }

 // Push then on the return stack
 if (RstackPush(context,limit))
      {
	  runtimeErrorMessage(context,"Return stack overflow in DO");
      return 0;
      }

 // Push then on the return stack
 if (RstackPush(context,index))
      {
	  runtimeErrorMessage(context,"Return stack overflow in DO");
      return 0;
      }

 // In case of +DO
 if (value==F_DO_PLUS)
        {
	    // Get address
	    addr=getAddrFromHere(context);
	    // Jump if needed
	    if (index>=limit) context->Counter=addr;
        }

 // In case of -DO
  if (value==F_DO_MINUS)
         {
 	    // Get address
 	    addr=getAddrFromHere(context);
 	    // Jump if needed
 	    if (index<=limit) context->Counter=addr;
         }

 return 0;
 }

// LOOP execute word    [HIDDEN]
int32_t ExecuteLOOP(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int32_t limit,index,addr;

 // Get address
 addr=getAddrFromHere(context);

 // Get the loop limit
 if (loopGetLimit(context,&limit))
      {
	  runtimeErrorMessage(context,"Indexing error in LOOP");
      return 0;
      }

 // Add one to the top of the return stack
 RstackAddTop(context,1);

 // Verify if we end the loop
 RstackGetTop(context,&index);
 if (index>=limit) return 0;

 // If we arrive here we need to go to previous loop
 context->Counter=addr;

 return 0;
 }

// @LOOP execute word        [HIDDEN]
int32_t ExecuteNewLOOP(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int32_t limit,index,addr,inc;

 // Get address
 addr=getAddrFromHere(context);

 // Get the loop limit
 if (loopGetLimit(context,&limit))
      {
	  runtimeErrorMessage(context,"Indexing error in @LOOP");
      return 0;
      }

 // Get the increment to add
 if (PstackPop(context,&inc))
      {
 	  runtimeErrorMessage(context,"Cannot get @LOOP increment");
      return 0;
      }

 // Add to the top of the return stack
 RstackAddTop(context,inc);

 // Verify if we end the loop
 RstackGetTop(context,&index);

 if (inc>=0)
     { if (index>=limit) return 0; }
    else
     { if (index<=limit) return 0; }

 // If we arrive here we need to go to previous DO
 context->Counter=addr;

 return 0;
 }

// UNLOOP execute word
// Normal word, not hidden
int32_t ExecuteUNLOOP(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int32_t data;

 if (RstackPop(context,&data))
     {
	 runtimeErrorMessage(context,"Return stack underflow in UNLOOP");
     return 0;
     }

 if (RstackPop(context,&data))
     {
	 runtimeErrorMessage(context,"Return stack underflow in UNLOOP");
     return 0;
     }

 return 0;
 }

// Execution of OF in a case         [HIDDEN]
// Includes DROP
int32_t ExecuteOF(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int32_t addr,tag,data;

 // Get address
 addr=getAddrFromHere(context);

 // Pop the value to check to
 if (PstackPop(context,&tag))
     {
	 runtimeErrorMessage(context,"Cannot get stack tag in OF");
	 return 0;
     }

 // Get the data to compare to without popping it
 if(PstackGetTop(context,&data))
     {
 	 runtimeErrorMessage(context,"Cannot get stack value in OF");
 	 return 0;
     }

 // If data equals tag continue run after drop
 if (data==tag)
      {
	  PstackPop(context,&data);
	  return 0;
      }

 // If we arrive here, jump over this block
 (context->Counter)=addr;

 return 0;
 }
