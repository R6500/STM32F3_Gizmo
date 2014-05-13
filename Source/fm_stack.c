/**************************************************
 *
 *  f m _ s t a c k . c
 *
 * Stack functions for the Forth project
 *
 * This file provide the implementation of the stacks
 * inside the context structure:
 *
 *     * Parameter stack
 *     * Return stack
 *
 *************************************************/

// Includes
#include "fp_config.h"     // Main configuration file
#include "fp_port.h"           // Main port definitions
#include "fm_main.h"           // Main forth include file
#include "fm_program.h"        // Program header file
#include "fm_screen.h"         // Screen header file
#include "fm_debug.h"          // Debug header file
#include "fm_stack.h"          // This module header file

/******************** PARAMETER STACK STATIC FUNCTIONS *************************/

// Stack Roll
// ( an ... a0 -- an-1 ... a0 an )
static void PstackRoll(ContextType *context,int32_t n)
 {
 int16_t *stkPointer; // Stack pointer
 int32_t i,value;

 // Return if nothing to do
 if (n<1) return;

 // Return if there is not enough stack depth
 if (n>=(context->stack.Size)) return;

 // Obtain stack pointer
 stkPointer=&(context->stack.Pointer);

 // Pick nth object
 value=context->stack.data[((*stkPointer)-n+STACK_SIZE)%STACK_SIZE];

 // Move down all n elements
 for(i=n-1;i>=0;i--)
	 context->stack.data[((*stkPointer)-i-1+STACK_SIZE)%STACK_SIZE]=
			 context->stack.data[((*stkPointer)-i+STACK_SIZE)%STACK_SIZE];

 // Set top element
 context->stack.data[(*stkPointer)]=value;
 }

/******************** PARAMETER STACK PUBLIC FUNCTIONS *************************/

// Initializes the parameter stack for the context
void PstackInit(ContextType *context)
 {
 context->stack.Pointer=-1;  // Next element will be 0
 context->stack.Size=0;      // We have no elements yet
 }

// Print the top of the selected context stack
// Prints also the number of elements on the stack
// This is a low level function that don't check VerboseLevel
void PstackPrintTop(ContextType *context)
 {
 int32_t value,size;

 if (context->stack.Pointer==-1)
	  consolePrintString("<Empty Stack>");
     else
      {
      size=PstackGetSize(context);
      value=context->stack.data[context->stack.Pointer];
      consolePrintf("<%d> Top: %d",size,value);
      }
 }

// Introduces a number on one the parameter stack
// As the parameter stack is circular, it never fails
void PstackPush(ContextType *context,int32_t value)
 {
 // Increment stack pointer
 (context->stack.Pointer)=((context->stack.Pointer)+1)%STACK_SIZE;

 // Increment current Stack Size if possible
 if ((context->stack.Size)<STACK_SIZE) (context->stack.Size)++;

 // Introduce value on the stack
 (context->stack.data)[context->stack.Pointer]=value;
 }

// Takes one element from the parameter stack of the context
// returns it using a pointer
// Returns 0 if there is no error
int32_t PstackPop(ContextType *context,int32_t *value)
 {
 int32_t pos; // Position to return data

 // Check if there is data
 if (!(context->stack.Size))
       {
	   runtimeErrorMessage(context,"Stack is empty");
	   return 1;
       }

 // Return data from current position
 pos=(context->stack.Pointer);

 // Recalculate Stack Pointer
 (context->stack.Pointer)=((context->stack.Pointer)+STACK_SIZE-1)%STACK_SIZE;

 // Recalculate stack size
 (context->stack.Size)--;

 // Verify if the Stack is now empty
 if (!(context->stack.Size)) (context->stack.Pointer)=-1;

 // Set return value
 (*value)=(context->stack.data)[pos];

 // Return without error
 return 0;
 }

int32_t PstackGetSize(ContextType *context)
 {
 return (context->stack.Size);
 }

// Gets Parameter Stack top without popping it
// Returns 0 if OK
//         1 if empty
int32_t PstackGetTop(ContextType *context,int32_t *value)
 {
 // Check if is empty
 if (!(context->stack.Size)) return 1;

 (*value)=(context->stack.data)[context->stack.Pointer];

 return 0;
 }

// Copies the stack from cBase to cCopy
void PstackClone(ContextType *cBase,ContextType *cCopy)
 {
 int32_t i;

 // Copy stack parameters
 cCopy->stack.Pointer=cBase->stack.Pointer;
 cCopy->stack.Size=cBase->stack.Size;

 // Copy stack data
 for(i=0;i<STACK_SIZE;i++)
	 (cCopy->stack).data[i]=(cBase->stack).data[i];
 }

/******************** PARAMETER STACK COMMAND FUNCTIONS *************************/

// Generic stack functions
// Don't include functions that generate one element from two
// STACK_F_DROP
// STACK_F_DUP
// STACK_F_DUP_INT // ?DUP
// STACK_F_PICK
// STACK_F_CLEAR
// STACK_F_DROP_N
// STACK_F_DUP_N
// STACK_F_SWAP_N
// STACK_F_ROT
// STACK_F_ROLL
int32_t PstackFunction(ContextType *context,int32_t value)
 {
 int32_t dummy,n,i;
 StackType *stk;      // Stack for this process
 int32_t *stkData;    // Stack data for this process
 int16_t *stkPointer; // Stack pointer for this process
 int16_t *stkSize;    // Stack size for this process

 // Obtain our stack pointers
 stk=&(context->stack);
 stkData=(stk->data);
 stkPointer=&(stk->Pointer);
 stkSize=&(stk->Size);

 switch (value)
     {
     case STACK_F_DROP:         // Remove top element
    	 PstackPop(context,&dummy);  // Just pop one value
    	 break;

     case STACK_F_DUP:  // Duplicate top element
    	 if (PstackGetSize(context)) // If there is data...
    	       // ...Duplicate it
    	       PstackPush(context,stkData[*stkPointer]);
       	 break;

     case STACK_F_DUP_INT:  // Duplicate top element if not zero
    	 if (PstackGetSize(context)) // If there is data...
    	    // ...Duplicate it
    		if (stkData[*stkPointer])
    	        PstackPush(context,stkData[*stkPointer]);
       	 break;

     case STACK_F_CLEAR:  // Clear the stack ------------
              (*stkPointer)=-1;
              (*stkSize)=0;
          break;

     case STACK_F_DROP_N:    // Removes n elements (excluding the n value)
    	 if (!PstackPop(context,&n))  // Try to get the n value
    		 if (n>0)  // See if n is positive
    	          {
    			  if (n>(*stkSize)) n=(*stkSize);  // Limit number of drops
    			  for(i=0;i<n;i++) PstackPop(context,&dummy);
    			  }
    	 break;

     case STACK_F_DUP_N:    // Duplicates the last n values (excluding n value)
    	 if (!PstackPop(context,&n))  // Try to get the n value
    		 if ((n>0)&&(n<=(*stkSize)))  // See if n is and less than stack size
    	          {
    			  for(i=0;i<n;i++)
    			       {
    				   PstackPush(context,stkData[((*stkPointer)-n+1+STACK_SIZE)%STACK_SIZE]);
    			       }
    	          }
    	 break;

     case STACK_F_PICK:    // Copies element -n to the top (excluding n value)
    	 if (!PstackPop(context,&n))  // Try to get the n value
    		 if ((n>=0)&&(n<(*stkSize)))  // See if n is positive and less than stack size
    		      {
    	          PstackPush(context,stkData[((*stkPointer)-n+STACK_SIZE)%STACK_SIZE]);
    	          }
    	 break;

     case STACK_F_ROT:    // Rotate the stack first three elements
    	 PstackRoll(context,2);
    	 break;

     case STACK_F_ROLL:    // Rolls the stack
    	 if (!PstackPop(context,&n))  // Try to get the n value
    		    PstackRoll(context,n);
    	 break;

     case STACK_F_SWAP_N:    // Interchanges element -n with the top top (excluing n value)
    	 if (!PstackPop(context,&n))  // Try to get the n value
    		 if ((n>0)&&(n<(*stkSize)))  // See if n>0 and less than stack size
    		    {
    			// Store current top value in temporal variable
    			dummy=stkData[*stkPointer];
    			// Determine position of element
    			i=((*stkPointer)-n+STACK_SIZE)%STACK_SIZE;
    			// Change top value
    			stkData[*stkPointer]=stkData[i];
    			// Change old position
    			stkData[i]=dummy;
    		    }
    	 break;

     case STACK_F_OVER:    // Pushes the 2nd element
    	 // Check if there if there is at least two elements
    	 if (PstackGetSize(context)<2) return 0;
    	 PstackPush(context,stkData[((*stkPointer)-1+STACK_SIZE)%STACK_SIZE]);
    	 break;

     case STACK_F_DEPTH:    // Gives stack size before call
    	 PstackPush(context,*stkSize);
    	 break;

     case STACK_F_TRUE:    // Pushes a true value
    	 PstackPush(context,FTRUE);
    	 break;

     case STACK_F_FALSE:    // Pushes a false value
    	 PstackPush(context,FFALSE);
    	 break;

     case STACK_F_UNUSED:    // Pushes User Dict Free Memory
    	 PstackPush(context,getUserMemory());
    	 break;

     case STACK_F_PAD:  // Show PAD address
    	 PstackPush(context,(int32_t)PAD_ADDRESS);
    	 break;
     //default:
    	 //DEBUG_MESSAGE("Cannot arrive to default in stackFunction");
     }

 return 0;
 }

// Generic dual stack functions
// Includes:
//     STACK_F_ADD    Add 2 elements  (a)(b) -> (a+b)
//     STACK_F_SUB    Subtract        (a)(b) -> (a-b)
//     STACK_F_MULT   Multiply        (a)(b) -> (a*b)
//     STACK_F_DIV    Divide          (a)(b) -> (a/b)
//     STACK_F_MOD    Modulus         (a)(b) -> (a%b)
//     STACK_F_SWAP   Stack Swap      (a)(b) -> (b)(a)
int32_t PstackDualFunction(ContextType *context,int32_t value)
 {
 int32_t first,second,dummy;
 StackType *stk;      // Stack for this process
 int32_t *stkData;    // Stack data for this process
 int16_t *stkPointer; // Stack pointer for this process
 int16_t *stkSize;    // Stack size for this process

 // Obtain our stack pointers
 stk=&(context->stack);
 stkData=(stk->data);
 stkPointer=&(stk->Pointer);
 stkSize=&(stk->Size);

 // Calculate stack elements
 first=(*stkPointer);
 second=((*stkPointer)+STACK_SIZE-1)%STACK_SIZE;

 // Check if there are at least two elements
 if ((*stkSize)<2)
     	{
	    runtimeErrorMessage(context,"Not enough elements");

     	// We return as we cannot do the selected operation
     	return 0;
     	}

 switch (value)
     {
     case STACK_F_ADD:  // Add top and element below
    	 stkData[second]=stkData[second]+stkData[first];
         (*stkPointer)=second;
    	 (*stkSize)--;
    	 break;

     case STACK_F_SUB:  // Substract top element
         stkData[second]=stkData[second]-stkData[first];
         (*stkPointer)=second;
         (*stkSize)--;
         break;

     case STACK_F_MULT:  // Multiply two elements ------------
         stkData[second]=stkData[second]*stkData[first];
         (*stkPointer)=second;
         (*stkSize)--;
         break;

     case STACK_F_DIV:  // Divide two elements ------------
         stkData[second]=stkData[second]/stkData[first];
         (*stkPointer)=second;
         (*stkSize)--;
         break;

     case STACK_F_MOD:  // Calculate modulus ------------
    	 stkData[second]=stkData[second]%stkData[first];
         (*stkPointer)=second;
         (*stkSize)--;
         break;

     case STACK_F_SWAP:  // Swap two elements ------------
    	 dummy=stkData[second];
    	 stkData[second]=stkData[first];
    	 stkData[first]=dummy;
   	     break;

     case STACK_F_NIP:  // Eliminate element below top ------------
    	 stkData[second]=stkData[first];
    	 (*stkPointer)=second;
    	 (*stkSize)--;
   	     break;

     case STACK_F_MAX:  // Calculate maximum ------------
    	 if (stkData[first]>stkData[second])
    		       stkData[second]=stkData[first];
         (*stkPointer)=second;
         (*stkSize)--;
         break;

     case STACK_F_MIN:  // Calculate minimum ------------
    	 if (stkData[first]<stkData[second])
    		       stkData[second]=stkData[first];
         (*stkPointer)=second;
         (*stkSize)--;
         break;

     case STACK_F_TUCK:
    	 PstackPop(context,&first);   // Pop two values
    	 PstackPop(context,&second);
    	 PstackPush(context,first);   // Push three values
    	 PstackPush(context,second);
    	 PstackPush(context,first);
    	 break;

     case STACK_F_DIV_MOD:  // Calculate modulus and division
    	 dummy=stkData[first];
    	 stkData[first]=stkData[second]/dummy;
    	 stkData[second]%=dummy;
         break;

     //default:
     //	 DEBUG_MESSAGE("Cannot arrive to default in StackDualFunction");
     }

 return 0;
 }

#ifdef VERTICAL_STACK_LIST
// Old vertical version
// Lists the stack contents
int32_t PstackList(ContextType *context,int32_t value)
 {
 int i,pos;

 StackType *stk;      // Stack for this process
 int32_t *stkData;    // Stack data for this process
 int16_t *stkPointer; // Stack pointer for this process
 int16_t *stkSize;    // Stack size for this process

 // Obtain our stack pointers
 stk=&(context->stack);
 stkData=(stk->data);
 stkPointer=&(stk->Pointer);
 stkSize=&(stk->Size);


 // Check verbose level and foreground operation
 if (NO_RESPONSE(context)) return 0;

 // Header
 consolePrintf("%sStack is holding %d elements%s",BREAK,(*stkSize),BREAK);

 // Exit if there are no elements
 if (!(*stkSize))
	 {
	 // Line break at the end
	 consolePrintf("%s",BREAK);
	 return 0;
	 }

 // List all stack elements from bottom to top
 for(i=(*stkSize)-1;i>=0;i--)
    {
	pos=((*stkPointer)+STACK_SIZE-i)%STACK_SIZE;
	consolePrintf("\t(%d):%d%s",i,stkData[pos],BREAK);
    }

 // Line break at the end
 consolePrintf("%s",BREAK);

 return 0;
 }
#else
// New horizontal version more similar to standard forth
// Lists the stack contents
int32_t PstackList(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int i,pos;

 StackType *stk;      // Stack for this process
 int32_t *stkData;    // Stack data for this process
 int16_t *stkPointer; // Stack pointer for this process
 int16_t *stkSize;    // Stack size for this process

 // Obtain our stack pointers
 stk=&(context->stack);
 stkData=(stk->data);
 stkPointer=&(stk->Pointer);
 stkSize=&(stk->Size);


 // Check verbose level and foreground operation
 if (NO_RESPONSE(context)) return 0;

 // Header
 consolePrintf("%s<%d> ",BREAK,(*stkSize));

 // Exit if there are no elements
 if (!(*stkSize))
	 {
	 // Line break at the end
	 consolePrintf("%s",BREAK);
	 return 0;
	 }

 // List all stack elements from bottom to top
 for(i=(*stkSize)-1;i>=0;i--)
    {
	pos=((*stkPointer)+STACK_SIZE-i)%STACK_SIZE;
	consolePrintf("%d ",stkData[pos]);
    }

 // Line break at the end
 consolePrintf("%s",BREAK);

 return 0;
 }
#endif


// Relational operators
// Includes:
//     REL_F_LESS
//     REL_F_GREATER
//     REL_F_L_EQUAL
//     REL_F_G_EQUAL
//     REL_F_EQUAL
//     REL_F_UNEQUAL
//     LOGIC_F_AND
//     LOGIC_F_OR
//     LOGIC_F_NOT
int32_t PstackRelationalFunction(ContextType *context,int32_t value)
 {
 int32_t a,b,c=0;

 // Try to pop two values
 if (PstackGetSize(context)<2)
     {
	 runtimeErrorMessage(context,"Not enough elements");
	 return 0;
     }

 PstackPop(context,&b); // Pop last entered value
 PstackPop(context,&a); // Pop last value

 switch (value)
   {
   case REL_F_LESS:  // Less than
	  c=(a<b);
	  break;

   case REL_F_GREATER:  // Greater than
	  c=(a>b);
	  break;

   case REL_F_L_EQUAL:  // Less or equal
	  c=(a<=b);
	  break;

   case REL_F_G_EQUAL:  // Greater or equal
	  c=(a>=b);
	  break;

   case REL_F_EQUAL:  // Equal
	  c=(a==b);
	  break;

   case REL_F_UNEQUAL:  // Unequal
	  c=(a!=b);
	  break;

//   case LOGIC_F_AND:  // Logic AND
//	  c=(a&&b);
//	  break;

//   case LOGIC_F_OR:  // Logic NOT
//	  c=(a||b);
//	  break;

   //default:
   //  DEBUG_MESSAGE("Cannot arrive to default in stackRelationalFunction");
   }

 if (c) // Push result on the stack
	 PstackPush(context,-1);
    else
     PstackPush(context,0);

 return 0;
 }

// Bitwise Logic operators
// Includes:
//     BIT_F_NOT
//     BIT_F_AND
//     BIT_F_OR
//     BIT_F_XOR
//     BIT_F_SHL
//     BIT_F_SHR
int32_t PstackBitwiseFunction(ContextType *context,int32_t value)
 {
 int32_t a,b,c=0;
 uint32_t *pa,*pb,*pc;

 // Associate unsigned with signed
 pa=(uint32_t*)&a;
 pb=(uint32_t*)&b;
 pc=(uint32_t*)&c;

 // Check if it is the the inversion
 if (value==BIT_F_NOT)
     {
	 // Try to pop one value
	 if (PstackPop(context,&a)) return 0;

	 (*pa)=~(*pa); // Calculate the operation

	 // Push result
	 PstackPush(context,a);

	 return 0;
     }

 // The rest of functions have two values

 // Try to pop two values
 if (PstackGetSize(context)<2)
     {
	 runtimeErrorMessage(context,"Not enough elements");
	 return 0;
     }

 PstackPop(context,&b); // Pop last entered value
 PstackPop(context,&a); // Pop last value

 switch (value)
   {
   case BIT_F_AND:  // And
	  (*pc)=((*pa)&(*pb));
	  break;

   case BIT_F_OR:  // Or
	  (*pc)=((*pa)|(*pb));
	  break;

   case BIT_F_XOR:  // Xor
	  (*pc)=((*pa)^(*pb));
	  break;

   case BIT_F_SHL:  // Shift left
	  (*pc)=((*pa)<<(*pb));
	  break;

   case BIT_F_SHR:  // Shift right
	  (*pc)=((*pa)>>(*pb));
	  break;

   //default:
   //  DEBUG_MESSAGE("Cannot arrive to default in stackBitwiseFunction");
   }

 // Push result on the stack
 PstackPush(context,c);

 return 0;
 }

// Functions that take one vale and return another

int32_t PstackUnaryFunction(ContextType *context,int32_t value)
 {
 int32_t *data;
 int16_t idata16;
 uint16_t *pudata16;
 int8_t idata8;
 uint8_t *pudata8;
 uint32_t *udata;

 // Set pointer locations
 pudata16=(uint16_t*)&idata16;
 pudata8=(uint8_t*)&idata8;

 // Check if there is anything on the stack
 if (!(context->stack.Size)) return 0;

 // Location of top of stack
 data=&(context->stack.data[context->stack.Pointer]);
 udata=(uint32_t*)data;

 switch (value)
   {
   case UN_F_NEGATE:
      (*data)=-(*data);
      break;

   case UN_F_NOT:
	  if ((*data)==0)
		  (*data)=-1;
	     else
	      (*data)=0;
      break;

   case UN_F_ABS:
	  if ((*data)<0) (*data)=-(*data);
      break;

   case UN_F_INC:
	  (*data)++;
      break;

   case UN_F_DEC:
	  (*data)--;
      break;

   case UN_F_INC2:
	  (*data)+=2;
      break;

   case UN_F_DEC2:
	  (*data)-=2;
      break;

   case UN_F_DUPLICATE:
	  (*data)*=2;
      break;

   case UN_F_HALVE:
	  (*data)/=2;
      break;

   case UN_F_0LESS:
	  if ((*data)<0) (*data)=-1; else (*data)=0;
      break;

   case UN_F_0GREAT:
	  if ((*data)>0) (*data)=-1; else (*data)=0;
      break;

   case UN_F_0EQUAL:
	  if ((*data)==0) (*data)=-1; else (*data)=0;
      break;

   case UN_F_0DIFF:
	  if ((*data)!=0) (*data)=-1; else (*data)=0;
      break;

   case UN_F_CELL_PLUS:
	  (*data)+=sizeof(int32_t);
      break;

   case UN_F_CELLS:
	  (*data)*=sizeof(int32_t);
      break;

   case UN_F_S16U:   // int16 -> uint16
	  idata16=(int16_t)(*data);
	  (*data)=(int32_t)(*pudata16);
      break;

   case UN_F_S8U:   // int8 -> uint8
	  idata8=(int8_t)(*data);
	  (*data)=(int32_t)(*pudata8);
      break;

   case UN_F_U16S:   // uint16 -> int16
	  (*pudata16)=(uint16_t)(*data);
	  (*data)=(int32_t)idata16;
      break;

   case UN_F_U8S:   // uint8 -> int8
	  (*pudata8)=(uint8_t)(*data);
	  (*data)=(int32_t)idata8;
	  break;

   case UN_F_USER2MEM:   // User addr to CPU addr
	  (*udata)=(uint32_t)(UDict.Mem+(*data));
	  break;

   case UN_F_MEM2USER:   // CPU addr to User addr
	  (*data)=(int32_t)((*udata)-(uint32_t)UDict.Mem);
	  break;

   case UN_F_HCELL_PLUS:
	  (*data)+=sizeof(int16_t);
      break;

   case UN_F_HCELLS:
	  (*data)*=sizeof(int16_t);
      break;

   }

 return 0;
 }

/******************** RETURN STACK PUBLIC FUNCTIONS *************************/

// Initializes the return stack for the context
void RstackInit(ContextType *context)
 {
 context->rstack.Pointer=-1;  // Next element will be 0
 }

// Introduces a number on one the return stack
// Returns 0 if ok
//         1 if overflows
int32_t RstackPush(ContextType *context,int32_t value)
 {
 // Check if we cannot accept more data
 if ((context->rstack.Pointer)>=(RSTK_SIZE-1))
       {
 	   if (SHOW_ERROR(context)) runtimeErrorMessage(context,"Rstack overflow");
 	   return 1;
       }

 // Increment stack pointer
 (context->rstack.Pointer)++;

 // Introduce value on the stack
 (context->rstack.data)[context->rstack.Pointer]=value;

 // Return OK
 return 0;
 }

// Takes one element from the return stack of the context
// returns it using a pointer
// Returns 0 if there is no error
int32_t RstackPop(ContextType *context,int32_t *value)
 {
 // Check if there is data
 if ((context->rstack.Pointer)<0)
       {
	   if (SHOW_ERROR(context)) runtimeErrorMessage(context,"Rstack is empty");
	   return 1;
       }

 // Return data from current position
 (*value)=(context->rstack.data)[context->rstack.Pointer];

 // Decrease pointer
(context->rstack.Pointer)--;

 // Return without error
 return 0;
 }

int32_t RstackGetSize(ContextType *context)
 {
 int32_t size;

 size=(context->rstack.Pointer)+1;

 return size;
 }

// Gets Return Stack top without popping it
// Returns 0 if OK
//         1 if empty
int32_t RstackGetTop(ContextType *context,int32_t *value)
 {
 // Check if is empty
 if ((context->rstack.Pointer<0))
     {
	 runtimeErrorMessage(context,"Rstack is empty");
	 return 1;
     }

 (*value)=(context->rstack.data)[context->rstack.Pointer];

 return 0;
 }

// Add a value to the top of the return stack
// Return 0 if OK
//        1 if return stack is empty
int32_t RstackAddTop(ContextType *context,int32_t value)
 {
 int32_t pointer;

 // Get return stack pointer
 pointer=context->rstack.Pointer;

 // Check if return stack is empty
 if (pointer<0)
	 {
	 runtimeErrorMessage(context,"Rstack is empty");
	 return 1;
	 }

 // Add value to top
 ((context->rstack.data)[pointer])+=value;

 return 0; // OK
 }


/******************** RETURN STACK COMMAND FUNCTIONS *************************/

#ifdef VERTICAL_STACK_LIST
// Old vertical version
// Lists the stack contents
int32_t RstackList(ContextType *context,int32_t value)
 {
 int32_t i,size;

 // Check verbose level and foreground operation
 if (NO_RESPONSE(context)) return 0;

 // Get size
 size=RstackGetSize(context);

 // Header
 consolePrintf("%sRstack is holding %d elements%s"
		       ,BREAK,size,BREAK);

 // Exit if there are no elements
 if (!size)
	 {
	 // Line break at the end
	 consolePrintf("%s",BREAK);
	 return 0;
	 }

 // List all stack elements from bottom to top
 for(i=0;i<size;i++)
    {
	consolePrintf("\t(%d):%d%s",size-i-1
			      ,(context->rstack.data)[i],BREAK);
    }

 // Line break at the end
 consolePrintf("%s",BREAK);

 return 0;
 }

#else
// New horizontal version more similar to standard forth
// Lists the stack contents
int32_t RstackList(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int32_t i,size;

 // Check verbose level and foreground operation
 if (NO_RESPONSE(context)) return 0;

 // Get size
 size=RstackGetSize(context);

 // Header
 consolePrintf("%sR:<%d> ",BREAK,size);

 // Exit if there are no elements
 if (!size)
	 {
	 // Line break at the end
	 consolePrintf("%s",BREAK);
	 return 0;
	 }

 // List all stack elements from bottom to top
 for(i=0;i<size;i++)
    {
	consolePrintf("%d ",(context->rstack.data)[i]);
    }

 // Line break at the end
 consolePrintf("%s",BREAK);

 return 0;
 }
#endif


// Generic return stack function
int32_t RstackFunction(ContextType *context,int32_t value)
 {
 int32_t data,size;

 switch (value)
     {
     case RSTACK_F_TO_R:     // Move from S to R
    	 if (PstackPop(context,&data)) return 0;
    	 RstackPush(context,data);
    	 break;

     case RSTACK_R_TO_F:     // Move from R to S
    	 if (RstackPop(context,&data)) return 0;
    	 PstackPush(context,data);
    	 break;

     case RSTACK_RTOP:     // Get R top without popping
     case RSTACK_GET_I:
    	 if (RstackGetTop(context,&data)) return 0;
    	 PstackPush(context,data);
    	 break;

     case RSTACK_GET_J: // Gets position -2
    	 size=RstackGetSize(context);
    	 if (size<3)
    	     {
    		 runtimeErrorMessage(context,"Not enough elements on return stack");
    		 return 0;
    	     }
    	 data=(context->rstack.data)[(context->rstack.Pointer)-2];
    	 PstackPush(context,data);
    	 break;

     case RSTACK_GET_K: // Gets position -4
    	 size=RstackGetSize(context);
    	 if (size<5)
    	     {
    		 runtimeErrorMessage(context,"Not enough elements on return stack");
    		 return 0;
    	     }
    	 data=(context->rstack.data)[(context->rstack.Pointer)-4];
    	 PstackPush(context,data);
    	 break;

     case RSTACK_CLEAR:     // Clear return stack
    	 RstackInit(context);
    	 break;

     case RSTACK_DROP:    // Removes top of return stack
    	 if (RstackPop(context,&data)) return 0;
      	 break;
     }

 return 0;
 }


