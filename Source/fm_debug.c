/*
 fm_debug.c
 Debug functions
 */

#include "fp_config.h"     // Main configuration file
#include "fp_port.h"     // Main port definitions
#include "fm_main.h"
#include "fm_debug.h"
#include "fm_program.h"
#include "fm_stack.h"
#include "fm_screen.h"

/******************** STATIC FUNCTIONS *******************************/

// Returns the printable character associated to
// and ascii number
static int32_t debugPrintable(int32_t character)
 {
 if ((character>=0x20)&&(character<=0x7E)) return character;

 return '.';
 }

/******************** PUBLIC FUNCTIONS *******************************/

// Send a string to the console
void debugMessage(char *cad)
 {
 // Return if debug is not enabled on verbose level
 if (!((MainContext.VerboseLevel)&VBIT_DEBUG)) return;

 // Send message to current console
 consolePrintf("DEBUG: %s%s",cad,BREAK);
 }

// Sends two strings to the console
void debugString(char *cad1, char *cad2)
 {
 // Return if debug is not enabled on verbose level
 if (!((MainContext.VerboseLevel)&VBIT_DEBUG)) return;

 // Send message to current console
 consolePrintf("DEBUG: %s%s%s",cad1,cad2,BREAK);
 }

// Sends a strings and an integer to the console
void debugInt(char *cad1, int32_t i)
 {
 // Return if debug is not enabled on verbose level
 if (!((MainContext.VerboseLevel)&VBIT_DEBUG)) return;

 // Send message to current console
 consolePrintf("DEBUG: %s%d%s",cad1,i,BREAK);
 }

// Dumps memory information on the console
// It don't check verbose level
// Parameters:
//    start : First memory position to dump
//    index : Position to show for the start position
//    size  : Number of bytes to dump
//    hex   : If true dump in hexadecimal, else in decimal
void debugDumpMemory(uint8_t *start, uint32_t index, int32_t size)
 {
 int32_t count=0,i;

 CBK; // End of line

 do
  {
  // Start of line
  if (MainFlags&MFLAG_DB_HEX)
      { consolePrintf("%10xh : ",index+count); }
     else
      { consolePrintf("%10u : ",index+count); }

  // Line hexadecimal or decimal data
  for(i=0;i<DEBUG_DUMP_LINESIZE;i++)
     if (MainFlags&MFLAG_DB_HEX)
        {
	    if ((count+i)<size)
	         { consolePrintf("%2x ",*(start+count+i)); }
	        else
	         { consolePrintf("   "); }
        }
       else
        {
	    if ((count+i)<size)
	         { consolePrintf("%3u ",*(start+count+i)); }
	        else
	         { consolePrintf("    "); }
        }

  // Separator for chars
  consolePrintf("| ");

  // Line char data
  for(i=0;i<DEBUG_DUMP_LINESIZE;i++)
	if ((count+i)<size)
		consolePutChar(debugPrintable(*(start+count+i)));

  CBK; // End of line

  // Increase counter
  count+=DEBUG_DUMP_LINESIZE;
  }
  while (count<size);

 CBK; // End of line
 };

/****************** COMMAND FUNCTIONS *******************************/

static void ShowFlag(int32_t flag)
 {
 if (flag)
   { consolePrintf(" TRUE%s",BREAK); }
  else
   { consolePrintf(" FALSE%s",BREAK); }
 }

int32_t DebugFunction(ContextType *context,int32_t value)
 {
 int32_t position,length;
 uint32_t *upos;

 // Set pointer
 upos=(uint32_t*)&position;

 switch (value)
   {
   case PF_F_DUMP: // Dump user memory from indicated position and length
	   if (PstackPop(context,&length)) return 0;
	   if (PstackPop(context,&position)) return 0;

	   // Return if response is not enabled on verbose level
	   if (!((context->VerboseLevel)&VBIT_RESPONSE)) return 0;

	   // Check parameters
	   if (length<=0) return 0;
	   if (position<0) return 0;
	   if (position>=UD_MEMSIZE) return 0;

	   // Limit to end of user memory
	   if (length>(UD_MEMSIZE-position))
		          length=UD_MEMSIZE-position;

	   // Call debug dump
	   debugDumpMemory(UDict.Mem+position,position,length);
	  break;

   case PF_F_PGMDATA: // Show program data
	   // Return if response is not enabled on verbose level
	   if (!((context->VerboseLevel)&VBIT_RESPONSE)) return 0;

	   consolePrintf("  Next position to code : %d%s",UDict.Base.nextPos,BREAK);
	   if (UDict.Base.lastWord==NO_WORD)
		       { consolePrintf("  Last coded word : NONE%s",BREAK); }
	         else
	           {
	           consolePrintf("  Last coded word (%d) : ",UDict.Base.lastWord);
	           showWordName((int32_t)UDict.Base.lastWord);
	           CBK;
	           }
	   consolePrintf("  Free memory : %d Bytes%s",getUserMemory(),BREAK);
	   consolePrintf("  Start word : ");
	   if (UDict.Base.startWord==NO_WORD)
	           { consolePrintf(" NOT DEFINED%s",BREAK); }
	          else
	           {
	           showWordName((int32_t)UDict.Base.startWord);
	           CBK;
	           }
	   consolePrintf("%s",BREAK);
	  break;

   case PF_F_MEMDUMP:   // Dump memory from indicated position and lenght
	   if (PstackPop(context,&length)) return 0;
	   if (PstackPop(context,&position)) return 0;

	   // Return if response is not enabled on verbose level
	   if (!((context->VerboseLevel)&VBIT_RESPONSE)) return 0;

	   if (length<=0) return 0;  // Check lenght

	   debugDumpMemory((uint8_t*)(*upos),position,length);
	   break;

   case PF_F_FLAGS: // Show static flag information
	   // TODO Check that there is no flag missing
	   // Return if response is not enabled on verbose level
	   if (!((context->VerboseLevel)&VBIT_RESPONSE)) return 0;
	   showCapabilities(); // Show first compile time capabilities
	   consolePrintf("Main Flags%s",BREAK);
	   consolePrintf("  User dictionary was loaded: ");
	   ShowFlag(MainFlags&MFLAG_LOADED);
	   consolePrintf("  Start word was executed: ");
	   ShowFlag(MainFlags&MFLAG_START_WORD);
	   consolePrintf("  Debug/Assert is active: ");
	   ShowFlag(MainFlags&MFLAG_DEBUG_ON);
	   consolePrintf("  Debug in hexadecimal: ");
	   ShowFlag(MainFlags&MFLAG_DB_HEX);
	   CBK;
	   break;

   case PF_F_DEC: // Debug in decimal
	   MainFlags&=~MFLAG_DB_HEX;
	   break;
   case PF_F_HEX: // Debug in hexadecimal
 	   MainFlags|=MFLAG_DB_HEX;
 	   break;

   case PF_F_LIMITS: // Show MForth limits
	   if (!((context->VerboseLevel)&VBIT_RESPONSE)) return 0;
       consolePrintf("MForth current limits:%s%s",BREAK,BREAK);
       consolePrintf("  Parameter stack size: %d cells%s",STACK_SIZE,BREAK);
       consolePrintf("  Return stack size: %d cells%s",RSTK_SIZE,BREAK);
       CBK;
       consolePrintf("  Pad size: %d bytes%s",PAD_SIZE,BREAK);
       CBK;
       consolePrintf("  Maximum console line length: %d chars%s",MAX_CONSOLE_LINE,BREAK);
       consolePrintf("  Maximum word name length: %d chars%s",MAX_TOKEN_SIZE,BREAK);
       CBK;
       consolePrintf("  Maximum user dictionary size: %d Bytes%s",UD_MEMSIZE,BREAK);
       consolePrintf("  Maximum number of locals in a word: %d%s",MAX_LOCALS,BREAK);
       CBK;
       consolePrintf("  Maximum number of background threads: %d%s",MAX_THREADS,BREAK);
       consolePrintf("  Maximum thread priority: %d%s",MAX_PRIO,BREAK);
       consolePrintf("  Minimum thread priority: %d%s",MIN_PRIO,BREAK);
       CBK;
       consolePrintf("  Int32 (Cell) ranges from %d to %d%s",MIN_4B_INT,MAX_4B_INT,BREAK);
       consolePrintf("  Int15 (Half Cell) ranges from %d to %d%s",MIN_2B_INT,MAX_2B_INT,BREAK);
       consolePrintf("  Int8 (Char) ranges from %d to %d%s",MIN_1B_INT,MAX_1B_INT,BREAK);
       CBK;
       portShowLimits();  // Add limit listing from PORT
	   break;
   };

 return 0;
 }


