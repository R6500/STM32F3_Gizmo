/***********************************************************************
 *
 *      f m _ m a i n . c
 *
 * Main forth functions
 *
 * Version 1.0 (17/3/2014)
 *
 ***********************************************************************/

// Includes
#include "fp_config.h"         // Main configuration file
#include "fp_port.h"           // Main port definitions
#include "fm_main.h"           // This module header file
#include "fm_stack.h"          // Stack module
#include "fm_program.h"        // Program module
#include "fm_screen.h"         // Screen module
#include "fm_register.h"       // Register module
#include "fm_threads.h"        // Threads module


// Variables ------------------------------------

// Context where the interpreter runs
ContextType MainContext;

// Context where the interrupts run
ContextType InterruptContext;

// Console line
char line[MAX_CONSOLE_LINE];

// Current line position used by tokenGet
char *linePointer=line;

// Parser status
uint32_t ParserStatus=0;
#define PS_COMMENT      (BIT(0))  // We are in a "(" comment
#define PS_DOT_COMMENT  (BIT(1))  // We are in a ".(" comment

// Parser mode
int32_t STATUS=0;  // True compile / False interactive

// Main flags
uint32_t MainFlags=0;

// Compile time capabilities
uint32_t Capabilities=0;

// External variables
extern int32_t  CompileLine;   // Defined in fm_program.c
extern int16_t localMode;      // Defined in fm_program.c

/************************* STATIC FUNCTIONS ***************************/

// Writes some start messages to the console
static void forthStartMessage(void)
 {
 // Check information verbose level
 if (!((MainContext.VerboseLevel)&VBIT_INFO)) return;

 CBK;
 consolePrintString("CForth"); CBK;

 consolePrintString("Version ");
 consolePrintString(FVERSION);
 consolePrintString(" (");
 consolePrintString(FVERSION_DATE);
 consolePrintString(")"); CBK;
 consolePrintf("Released under the GNU v3 public license%s",BREAK);

 consolePrintString("  Parameter stack size: ");
 consolePrintInt(STACK_SIZE); CBK;
 consolePrintString("  Return stack size: ");
 consolePrintInt(RSTK_SIZE); CBK; CBK;

 #ifndef RELEASE_VERSION
 consolePrintString("Test version. Not to be released"); CBK;
 #endif //RELEASE_VERSION

 if (MainFlags&MFLAG_LOADED)
     { consolePrintString("User dictionary has been loaded"); CBK; }

 if (MainFlags&MFLAG_START_WORD)
     { consolePrintString("Start word was executed"); CBK; }

 consolePrintString("Console ready..."); CBK; CBK;
 }

// Reads one line from the console
static void readOneLine(void)
 {
 int32_t character,firstChar,linePos=0;

 // Increase compile line
 // It's no problem if we are out of compile mode
 CompileLine++;

 // Erase MFLAG_IERROR
 MainFlags&=(~MFLAG_IERROR);

 // Show top element if verbose allows it...
 if ((MainContext.VerboseLevel)&VBIT_TOP)
		 //...and we are not compiling
		    if (EditWord==NO_WORD)
               {
 	           PstackPrintTop(&MainContext);
 	           CBK;
               }
 CBK;  // Break

 // Write the prompt at the start
 if ((MainContext.VerboseLevel)&VBIT_PROMPT)
    {
	// Head of prompt in compile mode
	if (EditWord!=NO_WORD)
	      consolePrintString("COMP");   // Prompt in compile mode

	// Head of prompt in compile error mode
	if (MainFlags&MFLAG_CERROR)
		  consolePrintString("CERR");

	// Head of prompt in file mode error
	if (MainFlags&MFLAG_FERROR)
		  consolePrintString("FERR");

	consolePrintString(">");  // End of prompt message
    }

 // Infinite loop
 firstChar=0;
 while(1)
   {
   // Get new character
   character=consoleGetChar();
   if (!firstChar)
	    firstChar=character;

   // Determine if it is a terminator char
   if ((character==10)||(character==13))
      	{
	    // If we are not at the start of the line
	   	if (linePos)
	   	      {
	   		  // Write the terminator sequence
	   		  if ((MainContext.VerboseLevel)&VBIT_ECHO) CBK;

	   		  // Add final null
	   		  line[linePos]=0;

	   		  // Return
	   		  return;
	   	      }
	   	     else
	   	      {
	   	      // Increase line count
	   	      if (character==firstChar)
	   	    	      CompileLine++;
	   	      }
       	}
 	   else
 	    {
		// Determine if it is a BackSpace (8) or DEL (127)
 		if ((character==8)||(character==127))
 		     {
 		     if (linePos)
 		  	 	{
 		  	    // Echoes the character
 		  	    if ((MainContext.VerboseLevel)&VBIT_ECHO)
 		  	          consolePutChar(character);
 		  	    linePos--; // Go back
 		  	 	}
 		  	 }
 		    else
 		     {
 		     // Echoes the character
 		     if ((MainContext.VerboseLevel)&VBIT_ECHO)
 		    	consolePutChar(character);

 		     // Adds the char to the line
 		     if (linePos<(MAX_CONSOLE_LINE-1))
 		    		    {
 		    		    // Adds the character
 		    		    line[linePos]=character;

 		    		    linePos++; // Increments line counter
 		    		    }
 		     }
 	    }

   }

 // It should never return from this point
 }

// Check if the token includes a number
// If it is a number:
//    Interactive mode: pushes it on the stack and returns
//    Compile mode: puts it in memory
//    Returns 0
// It it is not a number returns not zero
static int32_t tokenCheckNumber(char *token)
 {
 int32_t value,result;

 // Check if we have a number
 if (!((((*token)>='0')&&((*token)<='9'))
		  || ((token[0]=='-')&&(token[1]!=0))))
	                               return 1;

 // Convert string to integer
 result=Fatoi(token,&value);

 // Check that there are no errors
 if (result) return 2;

 if (!STATUS)  // Check if we are in interactive mode
     {
     // Push the number in interactive mode
     PstackPush(&MainContext,value);
     }
    else
     {
     // Compile number in compile mode
     programCodeNumber(value);
     }


 // Returns OK
 return 0;
 }

// Check if the token includes a character 'x'
// If it is a character:
//    Interactive mode: pushes it on the stack and returns
//    Compile mode: puts it in memory
//    Returns 0
// It it is not a character returns not zero
static int32_t tokenCheckChar(char *token)
 {
 int32_t value;

 // Check if we start with '
 if ((*token)!='\'') return 1;

 value=token[1];  // Get ASCII code of char

 // If negative, convert to positive (2's complement)
 if (value<0) value=256+value;

 if (!STATUS)  // Check if we are in interactive mode
     {
     // Push the number in interactive mode
     PstackPush(&MainContext,value);
     }
    else
     {
     // Compile number in compile mode
     programCodeNumber(value);
     }

 // Returns OK
 return 0;
 }

// Check if the token is in the base dictionary
// If it is:
//    Interactive mode: executes it
//    Compile mode: compiles it
//    Returns 0
// It it is not a in the base dictionary returns not zero
static int32_t tokenCheckBaseDict(char *token)
 {
 int32_t pos;

 // Search base dictionary
 pos=searchRegister((DictionaryEntry*) BaseDictionary,token);

 if (pos<0) return 1; // Not found

 // If it is found but cannot be directly compiled or run return also but with 0 value
 if (BaseDictionary[pos].flags&DF_NCOMPILE) return 0;

 // Check if we are in interactive mode...
 if (!STATUS)
   {
   // Check if it is a non interactive word
   if (BaseDictionary[pos].flags&DF_NI)
     {
	 if ((MainContext.VerboseLevel)&VBIT_ERROR)
	        {
	        consolePrintf("ERROR: Word %s cannot be used interactively%s",token,BREAK);
	        errorEpilogue(); // Mark this as an error
	        }
	 return 0; // Found but not applicable
     }
   // call this command if it can be used
   //DEBUG_STRING("Calling function: ",(char*)commands[pos].name);
   (BaseDictionary[pos].function)(&MainContext,BaseDictionary[pos].argument);

   return 0;
   }

 // If we are in compile mode...
 if (baseCode(token)) //...code this token
	   consoleErrorMessage(&MainContext,"Out of memory");

 // Returns OK
 return 0;
 }

// Check if the token is in the interactive dictionary
// If it is found executes it and returns 0
// It it is not a in the interactive dictionary returns not zero
static int32_t tokenCheckInteractiveDict(char *token)
 {
 int32_t pos;

 // Search base dictionary
 pos=searchRegister((DictionaryEntry*) InteractiveDictionary,token);

 if (pos<0) return 1; // Not found

 // call this command
 //DEBUG_STRING("Calling function: ",(char*)commands[pos].name);
 (InteractiveDictionary[pos].function)(&MainContext,InteractiveDictionary[pos].argument);

 // Returns OK
 return 0;
 }

// Check if the token is in the generator dictionary
// If it is found executes it and returns 0
// It it is not a in the generator dictionary returns not zero
static int32_t tokenCheckGeneratorDict(char *token)
 {
 int32_t pos;

 // Search base dictionary
 pos=searchRegister((DictionaryEntry*) GeneratorDictionary,token);

 if (pos<0) return 1; // Not found

 // call this command
 //DEBUG_STRING("Calling function: ",(char*)commands[pos].name);
 (GeneratorDictionary[pos].function)(&MainContext,GeneratorDictionary[pos].argument);

 // Returns OK
 return 0;
 }

// Check if the token is in the user dictionary
// If it is found executes it and returns 0
// It it is not a in the user dictionary returns not zero
static int32_t tokenCheckUserDict(char *token)
 {
 uint16_t pos;

 // Try to locate the word in the user dictionary
 pos=locateUserWord(token);

 if (pos==NO_WORD) return 1; // Not found

 // Check if we are in interactive mode...
 if (!STATUS)
   {
   // Execute at this position in main context
   programExecute(&MainContext,pos,1);

   return 0;
   }

 // If we are in compile mode..
 codeUserPosition(pos);  //..compile it

 // Returns OK
 return 0;
 }

// Process one token
static void processToken(char *token)
 {

 // If we have a compile error
 if (MainFlags&MFLAG_CERROR)
     {
	 if (!strCmp(token,";"))
	         {
	         // Erase MFLAG_ERROR
	         MainFlags&=(~MFLAG_CERROR);
	         }
	 return;
     }

 // If we have a file mode error
 if (MainFlags&MFLAG_FERROR)
     {
	 if (!strCaseCmp(token,"FEND"))
	         {
	         // Erase MFLAG_ERROR
	         MainFlags&=(~MFLAG_FERROR);
	         // Execute the FEND operation
	         ProgramFunction(&MainContext,PF_F_FEND);
	         }
	 // Ignore the token
	 return;
     }


 // If we have an assertion zone and assert or debug are not enabled
 if (!(MainFlags&MFLAG_DEBUG_ON))
		if (MainFlags&(MFLAG_ASSERT_ZONE|MFLAG_DEBUG_ZONE))
		   {
		   if (!strCmp(token,")"))
				    {
				    // End of assert zone
			        assertEnd(&MainContext,0);
				    }

		   // Don't process this token
		   return;
		   }

 // If we have a interpreter error leave processing
 if (MainFlags&MFLAG_IERROR) return;

 // Check if it is a print string
 if ((token[0]=='.')&&(token[1]=='\"'))
     {
	 // Check that the string is not empty
	 if (token[2]==0) return;

	 // Check if we are in interactive mode...
	 if (!STATUS)
	      {
		  // Interactive mode
		  consolePrintf("%s",token+2);
		  return;
	      }
	     else
	      {
	      // Compile mode
	      codePrintString(token+2);
	      return;
	      }
     }

 // Check if it is a normal string
 if (((token[0]=='S')||(token[0]=='s'))&&(token[1]=='\"'))
     {
	 // Check that the string is not empty
	 if (token[2]==0) return;

	 // Check if we are in interactive mode...
	 if (!STATUS)
	      {
		  // Interactive mode
		  consoleErrorMessage(&MainContext,"Strings cannot be used in interactive mode");
		  return;
	      }
	     else
	      {
	      // Compile mode
	      codeString(token+2);
	      return;
	      }
     }

 // Check if it is a normal alias "
 if (token[0]=='\"')
     {
	 // Check that the string is not empty
	 if (token[1]==0) return;

	 // Check if we are in interactive mode...
	 if (!STATUS)
	      {
		  // Interactive mode
		  consoleErrorMessage(&MainContext,"Strings cannot be used in interactive mode");
		  return;
	      }
	     else
	      {
	      // Compile mode
	      codeString(token+1);
	      return;
	      }
     }

 // Check length of token
 if (checkTokenLength(token)) return;

 // Local variable definition checks
 if ((STATUS)&&(localMode!=NO_LOCAL))  // If we are in compile and local mode..
  if (strCmp(token,"--")&&strCmp(token,"}")) //..and no local delimiter word..
     {
     if (localMode==LOCAL_ENTRY)   //..and entry mode..
              {
    	      codeLocalDefinition(token); //..code the word
    	      return;
              }

     if (localMode==LOCAL_COMMENT) //..and comment mode..
    	      return;  // ..ignore this word
     }

 // Local variables values
 if (STATUS) // Only in compile mode
   if (!tokenCheckLocal(token)) return;

 // If we are in interactive mode...
 if (!STATUS) //...check interactive dictionary
	 if (!tokenCheckInteractiveDict(token)) return;

 // Check if it is part of the User dictionary
 if (!tokenCheckUserDict(token)) return;

 // IF we are in compile mode...
 if (STATUS) //...check generator dictionary
      if (!tokenCheckGeneratorDict(token)) return;

 // Check if it is part of the Base dictionary
 if (!tokenCheckBaseDict(token)) return;

 // Check if it is a number
 if (!tokenCheckNumber(token)) return;

 // Check if it is a single character
 if (!tokenCheckChar(token)) return;

 // Check if verbose level allows error messages
 if (!((MainContext.VerboseLevel)&VBIT_ERROR)) return;

 // Give error message
 if (STATUS)
     {
     consolePrintf("ERROR: Token >>>%s<<< not recognized in compile mode%s",token,BREAK);
     abortCompile();  // Abort the compilation on error
     }
    else
     {
     consolePrintf("ERROR: Token >>>%s<<< not recognized in interactive mode%s",token,BREAK);
     errorEpilogue(); // Mark this as an error
     }
 }


// Converts a string in hexadecimal to an integer value
//    cad:   String to convert
//    value: Returned value
// Returns 0 if its OK and >0 in case of error
static int32_t atoiHex(char *cad,int32_t *value)
 {
 char *pointer;
 int32_t digit;
 int32_t first=1;
 uint32_t res;  // Unsigned result
 int32_t *pres; // Signed pointer to result

 // We know that we start with "0x" or "0X"

 pointer=cad+2; // Increase 2 chars from cad

 // Return error if there are no more chars
 if ((*pointer)==0) return 10;

 while ((*pointer)!=0) // If we have not reached the end
     {
	 digit=-1; // We have no digit yet
 	 // Convert digit

	 // Test if it is a number 0...9
	 if (((*pointer)>='0')&&((*pointer)<='9')) digit=(*pointer)-'0';

	 // Test if it is a letter a...f
	 if (((*pointer)>='a')&&((*pointer)<='f')) digit=(*pointer)-'a'+10;

	 // Test if it is a letter A...F
	 if (((*pointer)>='A')&&((*pointer)<='F')) digit=(*pointer)-'A'+10;

	 // If none of above generate error
	 if (digit==-1) return 11;

 	 // Check if this is the first digit
 	 if (first)
 		res=digit;  // It's the first digit
 	   else
 		res=res*16+digit;

 	 first=0; // No more first digits

 	 pointer++;  // Increase pointer
     }

 pres=(int32_t*)&res;
 (*value)=(*pres);    // Set return value as signed

 return 0;
 }

/************************* PUBLIC FUNCTIONS ***************************/

// Clear interrupt context
void clearInterruptContext(void)
 {
 // Clear stacks
 PstackInit(&InterruptContext);
 RstackInit(&InterruptContext);

 // Clear verbose level
 InterruptContext.VerboseLevel=0;

 // Clear all flags
 InterruptContext.Flags=0;
 }

// Check length of token
// Return 0 if OK
int32_t checkTokenLength(char *token)
 {
 // Check token length
 if (strLen(token)>MAX_TOKEN_SIZE)
       {
	   if (SHOW_ERROR(&MainContext))
	   {
	   consolePrintf("ERROR: Token >>>%s<<< longer than %d chars%s"
			         ,token,MAX_TOKEN_SIZE,BREAK);
	   }
	   errorEpilogue(); // Mark this as an error
	   return 1;
       }

 return 0;
 }

// Get a new token from input
char *tokenGet(void)
 {
 char *token;
 int32_t nQuotes=0;

 while (1)
   {
   // Check if we need a new line
   if (!(*linePointer))
       {
	   readOneLine();     // Read a line
	   linePointer=line;  // Start line pointer
       }

   // Check if we are inside a "(" comment
   if (ParserStatus&(PS_COMMENT|PS_DOT_COMMENT))
       {
       // Check if we end it
	   if ((*linePointer)==')')
		   ParserStatus&=(~(PS_COMMENT|PS_DOT_COMMENT));  // Take out state

	   // Echo in dot comment
	   if (ParserStatus&PS_DOT_COMMENT)
	               consolePutChar(*linePointer);

	   // Increase pointer
	   linePointer++;

	   // Jump to start of loop
	   continue;
       }

   // Skip spaces and tabs
   while (((*linePointer)==' ')||((*linePointer)=='\t'))
	      linePointer++;

   // Check if we are at the end of line
   if ((!(*linePointer))||((*linePointer)=='\\'))
           {
	       (*linePointer)=0;  // End this line
	       continue;          // Skip to next iteration
           }

   // Token start
   token=linePointer;

   // Search for end of token
   while(1)
     {
	 // Check for string delimiter
	 if ((*linePointer)=='\"')
	     {
		 nQuotes++; // Count double quotes

		 // End string at second quote
		 if (nQuotes==2)
		       {
			   (*linePointer)=0;  // Set end mark over last quote
			   linePointer++;     // Increase one character
			   return token;      // Get out of this function
		       }
	     }

	 // Check for end of token
	 if (!nQuotes)
	    if ( ((*linePointer)==' ')||((*linePointer)=='\t') )
	            {
	    	    (*linePointer)=0;  // Set end mark
	    	    linePointer++;     // Increase one character
	    		return token;      // Get out of this function
	            }

	 // Check if we are at the end of line
	 if ((!(*linePointer))||
		 (((*linePointer)=='\\')&&(!nQuotes))
		 )
	        {
	        (*linePointer)=0;  // Set end mark
	                           // Don't increase pointer to force new line read
	        return token;      // Get out of this function
	        }

	 // Increment token position
	 linePointer++;
	 }
   }

 // We should not return from this point
 return NULL;
 }

// Converts a string to an integer value
//    cad:   String to convert
//    value: Returned value
// Returns 0 if its OK and >0 in case of error
// Named Fatoi to not conflict with atoi in stdlib
int32_t Fatoi(char *cad,int32_t *value)
 {
 char *pointer;
 int32_t sign=1;  // No sign yet
 int32_t digit;   // One digit value
 int32_t res=0;   // Final result
 int first=1;     // True for the first digit

 // Start value of char pointer
 pointer=cad;

 // Check that the string is not empty
 if ((*cad)==0) return 1;

 // Check if the number is in hex...
 if ( (cad[0]=='0') && ((cad[1]=='x')||(cad[1]=='X')) )
      return atoiHex(cad,value); // Use another function in this case

 // Check if it is negative
 if ((*cad)=='-')
        {
	    sign=-1;    // Set sign
	    pointer++;  // Go to next char
        }

 while ((*pointer)!=0) // If we have not reached the end
    {
	// Convert digit
	digit=(*pointer)-'0';

	// Check digit
	if ((digit<0)||(digit>9)) return 2;

	// Check if this is the first digit
	if (first)
		res=digit;  // It's the first digit
	   else
		res=res*10+digit;

	first=0; // No more first digits

	pointer++;  // Increase pointer
    }

 res*=sign; // Add sign if needed

 (*value)=res; // Set return value

 return 0; // Return without error
 }

// Converts from integer to string using the indicated radix
// Modified from:
// https://www.fooe.net/trac/llvm-msp430/browser/trunk/mspgcc/msp430-libc/src/stdlib/itoa.c
// Sign is considered for all radix
char *Fitoa(int32_t num, char *str, int32_t radix)
 {
 int32_t sign = 0;   // To remember the sign if negative
 int32_t pos = 0;    // String position
 int32_t i;          // Generic use variable

 //Save sign
 if (num < 0)
       {
	   sign = 1;
       num = -num;
       }

 //Construct a backward string of the number.
 do {
    i = num % radix;
    if (i < 10)
         str[pos] = i + '0';
         else
         str[pos] = i - 10 + 'A';
    pos++;
    num /= radix;
    }
    while (num > 0);

 //Now add the sign
 if (sign)
       str[pos] = '-';
      else
       pos--;

 // Add the final null
  str[pos+1]=0;

 // Pos is now the position of the final digit (before null)

 // Now reverse the string
 i=0;
 do
  {
  sign=str[i];
  str[i++]=str[pos];
  str[pos--]=sign;
  }
  while(i<pos);

 return str;
 }

// Initializes the forth subsystem
void forthInit(void)
 {
 // Initializes the MainContext Stack
 PstackInit(&MainContext);

 // Initializes the MainContext Return Stack
 RstackInit(&MainContext);

 // Initializes the main context flags
 MainContext.Flags=0;

 // Initializes the main context process to foreground
 MainContext.Process=FOREGROUND;

 // Initializes the main context verbose level
 MainContext.VerboseLevel=VERBOSE_ALL;

 // Initializes the interrupt context
 InterruptContext.Flags=0;
 InterruptContext.Process=FOREGROUND;
 InterruptContext.VerboseLevel=0;

 // Initializes the forth program
 programInit();

 // Initializes the threads if enabled
 #ifdef USE_THREADS
 threadsInit();
 #endif //USE_THREADS

 // Check if there is a start word...
 if (UDict.Base.startWord!=NO_WORD)
	if (!(PORT_ABORT)) //...and abort is not active
         {
	     programExecute(&MainContext,UDict.Base.startWord,1);
	     // Set flag
	     MainFlags|=MFLAG_START_WORD;
         }

 }

// Starts the forth interpreter
void forthMain(void)
 {
 char *token;

 // Writes some information to the console
 forthStartMessage();

 // We need a new line
 (*line)=0;

 // Infinite read line loop
 while(1)
   {
   token=tokenGet();
   processToken(token);
   }
 }

// Aborts current read line
// This is called on error
void abortLine(void)
 {
 if (MainFlags&MFLAG_FILE)	 // In file mode...
     {
	 // Sets the file mode flag
	 MainFlags|=MFLAG_FERROR;
	 // Changes verbose
	 MainContext.VerboseLevel=FILE_ERROR_VERBOSE;
     }
    else  // Out of file mode
     {
     // Sets the interpretation error flag
     MainFlags|=MFLAG_IERROR;
     }
 }

/******************* COMMAND FUNCTIONS ******************/

// Immediate function
// In reality they are not exclusively immediate but interactive
int32_t immediateFunction(ContextType *context,int32_t value)
 {
 switch (value)
    {
    case IMM_F_COMMENT:  // Comment "("
       ParserStatus|=PS_COMMENT;   // Change parser status
	   break;

    case IMM_F_DOT_COMMENT:  // Comment ".("
       ParserStatus|=PS_DOT_COMMENT;   // Change parser status
	   break;

    case IMM_F_GO_COMPILE:  // Implements "]"
       // Check that we are editing a word
       if (EditWord==NO_WORD)
             {
    	     consoleErrorMessage(context,"We are not editing any word");
    	     return 0;
             }
       STATUS=1;
	   break;
    }

 return 0;
 }

// Shows the capabilities associated to current compilation
void showCapabilities(void)
 {
 consolePrintf("Compile time capabilities%s",BREAK);

 #ifdef   USE_FDEBUG
 consolePrintf("  Debug is enabled%s",BREAK);
 #else  //USE_FDEBUG
 consolePrintf("  Debug is disabled%s",BREAK);
 #endif //USE_FDEBUG

 #ifdef   USE_THREADS
 consolePrintf("  Threads are enabled%s",BREAK);
 #else  //USE_THREADS
 consolePrintf("  Threads are disabled%s",BREAK);
 #endif //USE_THREADS
 }





