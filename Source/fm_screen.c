/**************************************************
 *
 *  f m _ s c r e e n . c
 *
 * Screen functions for the Forth project
 *
 * This file provide the implementation of screen functions
 * not directly related to the port console module
 *
 * These functionalities depend on escape sequences
 * and they don't work in windows command windows
 *
 * They should operate ok in linux and on terminal emulators
 *
 *************************************************/

// Includes
#include "fp_config.h"     // Main configuration file
#include "fp_port.h"           // Main port definitions
#include "fm_main.h"           // Main forth include file
#include "fm_stack.h"          // Stack header file
#include "fm_program.h"        // Program header file
#include "fm_register.h"
#include "fm_screen.h"         // This module header file

/********************** CONSTANTS ****************************/

// Control Sequence Introducer String (ANSI)
const char CSIstring[]={27,'[',0};

// Possible line break sequences
const char BRK_MATRIX[3][3]={"\r\n","\r","\n"};

/********************** VARIABLES ***************************/

char *BREAK=BREAK_DEFAULT;

// External variables
extern uint32_t EditWord;  // Defined in fm_program.c

/******************* STATIC FUNCTIONS ******************/

/**************** PUBLIC CONSOLE FUNCTIONS ******************/

// Procedures to implement in each
// console error function
void errorEpilogue(void)
 {
 if (EditWord==NO_WORD)
     {
	 // We are in interpreter mode
	 abortLine();
     }
    else
     {
	 // We are in compile mode or in a [ ] zone
     abortCompile();
     }
 }

// Sends a new line terminator
void consoleBreak(void)
 {
 consolePrintf(BREAK);
 }

// Prints one string on the console
void consolePrintString(char *cad)
 {
 consolePrintf(cad);
 }

// Prints one int32_t on the console
void consolePrintInt(int32_t value)
 {
 consolePrintf("%d",value);
 }

// Send an error message to the console
// In compilation mode aborts compilation
// In interpreter mode aborts the rest of the line
void consoleErrorMessage(ContextType *context,char *cad)
 {
 errorEpilogue();

 // Return if we don't show errors
 if (NO_ERROR(context)) return;

 // Send message to current console
 consolePrintf("ERROR: %s%s\n",cad,BREAK);
 }

// Send an error message to the console followed by a string
void consoleErrorString(ContextType *context,char *cad1, char *cad2)
 {
 errorEpilogue();

 // Return if we don't show errors
 if (NO_ERROR(context)) return;

 // Send message to current console
 consolePrintf("ERROR: %s%s%s",cad1,cad2,BREAK);
 }

// Send an error message to the console followed by an integer
void consoleErrorInt(ContextType *context,char *cad1, int32_t value)
 {
 errorEpilogue();

 // Return if we don't show errors
 if (NO_ERROR(context)) return;

 // Send message to current console
 consolePrintf("ERROR: %s%d%s",cad1,value,BREAK);

 errorEpilogue();
 }

// Send an warning message to the console
void consoleWarnMessage(ContextType *context,char *cad)
 {
 // Return if we don't show errors
 if (NO_ERROR(context)) return;

 // Send message to current console
 consolePrintf("WARNING: %s%s\n",cad,BREAK);
 }

// Send an error message to the console
// Used for runtime errors
// It also aborts execution
void runtimeErrorMessage(ContextType *context,char *cad)
 {
 // Abort execution
 AbortExecution(context);

 // Set also normal error epilogue
 errorEpilogue();

 // Return if we don't show errors
 if (NO_ERROR(context)) return;

 // Send message to current console
 consolePrintf("RUN ERROR: %s%s\n",cad,BREAK);
 }

/******************* COMMAND FUNCTIONS **********************/

/* Set color (color)->
       Valid colors:
         0 : Default
         1 : Black
         2 : Red
         3 : Green
         4 : Yellow
         5 : Blue
         6 : Magenta
         7 : Cyan
         8 : Whyte
     11-18 : High intensity (aixterm)
       100 : Default background
   101-108 : Background colors */
int32_t screenColor(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int32_t data;
 char cColor[10]={27,'[','3','9','m',0};   // String for color coding
 char cbColor[10]={27,'[','3','0',';','1','m',0};   // String for bold color coding
 char rColor[10]={27,'[','0','m',0};   // String for reset color coding

 // Try to pop color data
 if (PstackPop(context,&data)) return 0;

 // Check that we have the console and can response
 if (NO_RESPONSE(context)) return 0;

 // Check if it is default
 if (data==0) {consolePrintf("%s",rColor); return 0;};

 // Check if it is default background
 if (data==100)
         {
   	     cColor[2]='4'; cColor[3]='9';
     	 consolePrintf("%s",cColor);
     	 return 0;
     	 }

 // Check if it is normal foreground
 if ((data>0)&&(data<9))
        {
        cColor[3]='0'+data-1;  // Code color
        consolePrintf("%s",cColor);
        return 0;
        }

 // Check if it is high intensity foreground (aixterm)
 if ((data>10)&&(data<19))
        {
        cColor[2]='9';
        cColor[3]='0'+data-11;  // Code color
        consolePrintf("%s",cColor);
        return 0;
        }

 // Check if it is bold color
 if ((data>20)&&(data<29))
        {
        cbColor[3]='0'+data-21;  // Code color
        consolePrintf("%s",cbColor);
        return 0;
        }

 // Check if it is normal foreground
 if ((data>100)&&(data<109))
       {
   	   cColor[2]='4';
       cColor[3]='0'+data-101;  // Code color
       consolePrintf("%s",cColor);
       return 0;
       }

 // If we arrive here the color is invalid
 runtimeErrorMessage(context,"Invalid color code");

 return 0;
 }

// Basic screen function
int32_t screenFunction(ContextType *context,int32_t value)
 {
 uint32_t udata;
 int32_t len,data,npad;

 char buffer[16];

 // Screen functions don't work if we are not in the foreground
 // Moved to NO_RESPONSE macro
 //if ((context->Process)!=FOREGROUND) return 0;

 switch (value)
   {
   case SF_F_PAGE:
	  if (NO_RESPONSE(context)) return 0;  // Exit if response not enabled
	  consolePrintf("%s2J",CSIstring);
	  consolePrintf("%s%d;%dH",CSIstring,1,1);
      break;

   case SF_F_CR:  // Prints line break
	  if (NO_RESPONSE(context)) return 0;  // Exit if response not enabled
 	  CBK;
      break;

   case SF_F_SPACE:  // Prints a space
	  if (NO_RESPONSE(context)) return 0;  // Exit if response not enabled
 	  consolePrintf(" ");
      break;

   case SF_F_BACKSPACE:  // Prints a backspace
	  if (NO_RESPONSE(context)) return 0;  // Exit if response not enabled
 	  consolePrintf("\b");
      break;

   case SF_F_CSI:  // Prints Control Sequence Introducer (ANSI)
	  if (NO_RESPONSE(context)) return 0;  // Exit if response not enabled
 	  consolePrintf("%s",CSIstring);
      break;

   case SF_F_VERBOSE: // Set verbose level
	  // Try to pop one value
	  if (PstackPopUnsigned(context,&udata)) return 0;
	  (context->VerboseLevel)=udata;  // Set verbose level
	  break;

   case SF_F_INTPADDED: // Implements .R
	  if (PstackGetSize(context)<2)  // Check if there are enough parameters
	      {
		  runtimeErrorMessage(context,"Not enough stack elements");
		  return 0;
	      }
	  PstackPop(context,&npad);  // Get pad number
	  PstackPop(context,&data);  // Get data to show
	  // Check verbose level
	  if (NO_RESPONSE(context)) return 0;
	  // Convert to string
	  Fitoa(data,buffer,10);
	  len=strLen(buffer);
	  while (npad>len)
	     { consolePutChar(' '); len++;}
	  consolePrintf("%s",buffer);
	  break;

   case SF_F_HEXPADDED: // Implements X.R
	  if (PstackGetSize(context)<2)  // Check if there are enough parameters
	      {
		  runtimeErrorMessage(context,"Not enough stack elements");
		  return 0;
	      }
	  PstackPop(context,&npad);  // Get pad number
	  PstackPop(context,&data);  // Get data to show
	  // Check verbose level
	  if (NO_RESPONSE(context)) return 0;
	  // Convert to string
	  Fitoa(data,buffer,16);
	  len=strLen(buffer);
	  while (npad>len)
	     { consolePutChar(' '); len++;}
	  consolePrintf("%s",buffer);
	  break;

   }

 return 0;
 }

// Screen function that gets at least one parameter from the stack
int32_t screenFunctionStack(ContextType *context,int32_t value)
 {
 int32_t data,data2;
 uint32_t *udata;
 int32_t i;

 // Screen functions don't work if we are not in the foreground
 // Moved to NO_RESPONSE macro
 // if ((context->Process)!=FOREGROUND) return 0;

 // Set pointer
 udata=(uint32_t*)&data;

 // Try to get data from the stack
 if (PstackPop(context,&data)) return 0;

 switch (value)
   {
   case SFS_F_DOT:   // Prints stack top
	  if (NO_RESPONSE(context)) return 0;  // Exit if response not enabled
	  consolePrintf("%d ",data);
      break;

   case SFS_F_DOTHEX:  // Prints stack top in hexadecimal
	  if (NO_RESPONSE(context)) return 0;  // Exit if response not enabled
 	  consolePrintf("%x ",*udata);
      break;

   case SFS_F_DOTU:  // Prints stack top as unsigned
	  if (NO_RESPONSE(context)) return 0;  // Exit if response not enabled
 	  consolePrintf("%u ",*udata);
      break;

   case SFS_F_SPACES:  // Prints <data> spaces
	  if (NO_RESPONSE(context)) return 0;  // Exit if response not enabled
	  if (data>0)
		  for(i=0;i<data;i++)
 	             consolePrintf(" ");
      break;

   case SFS_F_ATXY:  // Go to x,y position from 0,0
      // Try to get more data from the stack
      if (PstackPop(context,&data2)) return 0;

      if (NO_RESPONSE(context)) return 0;  // Exit if response not enabled

      // Check range
      if ((data<0)||(data2<0)) return 0;

      // Go to this position
      consolePrintf("%s%d;%dH",CSIstring,data+1,data2+1);
      break;

   case SFS_F_EMIT:  // Prints one character
	  if (NO_RESPONSE(context)) return 0;  // Exit if response not enabled
	  consolePrintf("%c",data);
      break;

   case SFS_F_SETBREAK:  // Sets line break sequence
	  if ((data<0)||(data>2))
		  {
		  if (SHOW_ERROR(context))
			  runtimeErrorMessage(context,"Invalid break sequence parameter");
		  return 0;
		  }
	  // Set new break
	  BREAK=(char*)BRK_MATRIX+data;
      break;
   }

 return 0;
 }
