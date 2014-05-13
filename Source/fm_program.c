/*******************************************************************
 *
 *  f m _ p r o g r a m . c
 *
 * Program functions for the Forth project
 *
 * This module implements the user dictionary
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
#include "fm_branch.h"
#include "fm_threads.h"
#include "fm_program.h"    // This module header file

// User dictionary
UserDictionary  UDict __attribute__ ((aligned (4)));

// Variables used during compilation ---------------------------------

uint16_t CodePosition;      // Position where we will insert next code
uint32_t EditWord=NO_WORD;  // Current compiled word position
int32_t  CompileLine;       // Compilation line for error messages

// Local variables

// Array that hold local names
char LocalNames[MAX_LOCALS][MAX_TOKEN_SIZE+1];
// Array to hold local indexes
uint8_t LocalIndex[MAX_LOCALS];
// Next local to process
int16_t nextLocal;
// First local in a variable local group
int16_t firstLocalInGroup;
// Local mode
int16_t localMode;
// Last verbose level before file mode start
uint32_t lastVerbose;


// -------------------------------------------------------------------

// Decode position for the "SEE" word
int32_t decodePosition;

// External variable dictionaries in fm_register.c
extern int32_t STATUS;   // True -> Compile mode

//extern uint32_t VerboseLevel;  // Defined in fm_main.c

/***************** STATIC FUNCTIONS *************************/

// Static variables to do word searches
static uint16_t searchWord,previousWord2,previousWord;
static char wordString[MAX_TOKEN_SIZE+1];

// Start a word search in user dictionary
static void startWordSearch(void)
 {
 // Start with the last added word
 searchWord=UDict.Base.lastWord;

 // There is no previous word
 previousWord2=NO_WORD;
 }

// Get the next word
// Returns null or a pointer to word string
// Returns by pointer the position of the word in memory if there is any
static char *nextWordSearch(uint16_t *position)
 {
 uint16_t *nextWord;
 int16_t   strLen,i=0;


 // Check if we are at the end
 if (searchWord==NO_WORD) return NULL;

 // Set position for this word
 (*position)=searchWord;

 // Get data if it exist
 nextWord=(uint16_t*)&(UDict.Mem[searchWord-2]);
 strLen=UDict.Mem[searchWord-3];

 // Copy string
 if (strLen)
	 for(i=0;i<strLen;i++)
		 wordString[i]=(char)UDict.Mem[searchWord-3-strLen+i];

 wordString[i]=0;  // Null terminator

 // Link to next word
 previousWord=previousWord2;
 previousWord2=searchWord;
 searchWord=(*nextWord);

 // Return pointer to the string
 return wordString;
 }

// Give help for one word   [USE IN INTERACTIVE MODE]
// Don't check for not entry
static void wordHelp(DictionaryEntry *dict)
 {
 char *pointer;

 // Don't check verbose

 CBK;  // Set line break

 // Show word namme
 consolePrintf("   %s",dict->name);

 // Check if it is a directive
 if ((dict->flags)&DF_DIRECTIVE)
 		 consolePrintf(" <word>  ");

 // Show flags

 if ((dict->flags)&DF_NI)
		 consolePrintf("   [PROGRAM ONLY]");

 if ((dict->flags)&DF_ADDR)
 		 consolePrintf("   [ADDR FOLLOWS]");

 CBK;  // Set line break

 // Show help data
 consolePrintf("   ");
 pointer=dict->help;
 while ((*pointer)!=0)
    {
	if ((*pointer)=='#')
		{ consolePrintf("%s   ",BREAK); }
	   else
	    {
		if ((*pointer)=='$')
			{ consolePrintf(" -- "); }
		   else
	        { consolePutChar(*pointer); }
	    }
	pointer++;
    }

 CBK; CBK;  // End of help
 }

// Backtraces one UDict position
static void backtracePosition(ContextType *context,int32_t position)
 {
 if (NO_ERROR(context)) return;

 // Return if we have no console
 if (NO_CONSOLE) return;

 consolePrintf("Backtrace: %d >> ",position);
 showWordName(position);
 consolePrintf(" <<%s",BREAK);
 }


/***************** STATIC CODING FUNCTIONS ******************/

// Code a int8_t byte at current position
// Returns 0 if it works ok
//         2 if there is no space to fit the number
static int32_t int8Code(int8_t value)
 {
 int8_t *pointer;

 // Check is there is space
 if ((CodePosition+1)>=UD_MEMSIZE) return 2;

 // Code the 1 Byte number command
 baseCode("1B_NUM");

 // Set pointer
 pointer=(int8_t*)(UDict.Mem+CodePosition);

 // Set value
 (*pointer)=value;

 // Increase counter
 CodePosition++;

 return 0;
 }

// Code a int16_t byte at current position
// Returns 0 if it works ok
//         2 if there is no space to fit the number
static int32_t int16Code(int16_t value)
 {
 int16_t *pointer;

 // Check is there is space
 if ((CodePosition+2)>=UD_MEMSIZE) return 2;

 // Code the 2 Byte number command
 baseCode("2B_NUM");

 // Set pointer
 pointer=(int16_t*)(UDict.Mem+CodePosition);

 // Set value
 (*pointer)=value;

 // Increase counter
 CodePosition+=2;

 return 0;
 }

// Code a int32_t byte at current position
// Returns 0 if it works ok
//         2 if there is no space to fit the number
static int32_t int32Code(int32_t value)
 {
 int32_t *pointer;

 // Check is there is space
 if ((CodePosition+4)>=UD_MEMSIZE) return 2;

 // Code the 4 Byte number command
 baseCode("4B_NUM");

 // Set pointer
 pointer=(int32_t*)(UDict.Mem+CodePosition);

 // Set value
 (*pointer)=value;

 // Increase counter
 CodePosition+=4;

 return 0;
 }

// Aborts from port PORT_ABORT defined in fp_port.h
static void portAbort(ContextType *context)
 {
 // Give this message only one time
 if (!((context->Flags)&CFLAG_ABORT))
	 runtimeErrorMessage(context,"User Abort");
 };


/***************** PUBLIC FUNCTIONS *************************/

// Locates a user word and returns its position
// Returns NO_WORD if it is not found
uint16_t locateUserWord(char *name)
 {
 char *nWord;
 uint16_t pos;

 // Start new word search
 startWordSearch();

 // Locate next word
 while ((nWord=nextWordSearch(&pos))!=NULL)
     {
	 // Check if we have found the word
	 if (!strCaseCmp(name,nWord)) return pos;
     };

 // Not found
 return NO_WORD;
 }

// Prints user dictionary word list
// Don't check verbose level
void userWordList(void)
 {
 char *name;
 int count=0,len,i;
 uint16_t pos=0;

 // Header text
 consolePrintf("%s%sUser dictionary:%s%s",BREAK,BREAK,BREAK,BREAK);

 // Start word search
 startWordSearch();

 // Fist word
 name=nextWordSearch(&pos);

 while (name!=NULL)
   {
   if (!count) consolePrintf("  ");
   consolePrintf("%s",name);

   // Add padding
   len=strLen(name);

   if (len<20)
	   for(i=0;i<20-len;i++)
	      { consolePrintf(" "); }

   count++;
   if (count>=4)
	   {
       count=0;
       CBK;
       }

   // Go to next word
   name=nextWordSearch(&pos);
   }

 if (!pos) consolePrintf("  <EMPTY>%s",BREAK);

 consolePrintf("%s",BREAK);
 }

// Clears all program data
// Don't touch port information
void programErase(void)
 {
 int32_t i;

 // Initialize base data
 UDict.Base.magic=MAGIC_NUMBER;
 UDict.Base.lastWord=NO_WORD;  // There is no last word
 UDict.Base.nextPos=0;         // We will start at position zero
 UDict.Base.startWord=NO_WORD; // We have no start word yet

 // Initialize data zone to all 1's
 for(i=0;i<(UD_MEMSIZE/4)-1;i++)
	   (*((uint32_t*)&(UDict.Mem[i*4])))=0xFFFFFFFF;

 }

// Initializes the program functions
// Port information won't be updated
void programInit(void)
 {
 PortSave *psp;	 // Pointer to port save data

 // Initialize port data (if any)
 psp=&(UDict.Port);
 portSaveInit(psp);

 // Program erase
 programErase();

 // Try to load from previous save
 loadUserDictionary();
 }

// Print on the console the word associated to an execute address
// Used by backtracePosition
// Parameter:
//     addr contains the address of execution
//          inside UDict
void showWordName(int32_t addr)
 {
 int32_t strLen,i;

 strLen=UDict.Mem[addr-3];

 // Copy string
 if (strLen)
	 for(i=0;i<strLen;i++)
		 consolePutChar((int32_t)UDict.Mem[addr-3-strLen+i]);
 }

/********************* PUBLIC CODING FUNCTIONS ***************************/

// Restores UDict to the state before the new word coding started
static void abortCoding(void)
 {
 int32_t i;

 // Restore memory
 if (CodePosition>UDict.Base.nextPos)
	   for(i=UDict.Base.nextPos;i<CodePosition;i++)
		   UDict.Mem[i]=0xFF;

 if (MainFlags&MFLAG_FILE) // In file mode...
     {
	 // Sets the file mode flag
	 MainFlags|=MFLAG_FERROR;
     }
    else  // Out of file mode
     {
     // Sets the flag
     MainFlags|=MFLAG_CERROR;
     }
 }

// Aborts compilation of current word
// Sets to ignore also the rest of tokens all
// the way to ";"
// Sets the MFLAG_CERROR MainFlag
void abortCompile(void)
   {
   // Restore UDict status
   abortCoding();

   STATUS=0;            // Go to interactive mode
   EditWord=NO_WORD;    // No word is current

   // Erase both stacks
   PstackInit(&MainContext);
   RstackInit(&MainContext);

   // Give error message
   consolePrintf("Compilation aborted at line %d%s",CompileLine,BREAK);
   }


// Add the indicated word from the Base dictionary in the
// current compiling position and increments the pointer
// Returns 0 if it works ok
//         1 if the word cannot be found
//         2 if there is no space to fit the word
//       100 if we are out of codes
int32_t baseCode(char *word)
 {
 int32_t pos;

 // Search the word in the dictionary
 pos=searchRegister((DictionaryEntry*)BaseDictionary,word);

 // Check if it is found
 if (pos==-1) return 1;

 // Check is there is space
 if (CodePosition>=UD_MEMSIZE) return 2;

 // Check if it is a 2 byte code in extend 1 250...499
 if ((pos>=EXT1_START)&&(pos<=EXT1_END))
     {
	 baseCode("EXT1");
	 UDict.Mem[CodePosition++]=pos-EXT1_START;
	 return 0;
     }

 // Check if it is a 2 byte code in extend 2 500...749
 if ((pos>=EXT2_START)&&(pos<=EXT2_END))
     {
	 baseCode("EXT2");
	 UDict.Mem[CodePosition++]=pos-EXT2_START;
	 return 0;
     }

 // Check if it is a 2 byte code in extend 3 750...999
 if ((pos>=EXT3_START)&&(pos<=EXT3_END))
     {
	 baseCode("EXT3");
	 UDict.Mem[CodePosition++]=pos-EXT3_START;
	 return 0;
     }

 if (pos>EXT3_END)
     {
 	 consoleErrorMessage(&MainContext,"Out of extend3 codes (FATAL)");
 	 abortCompile();  // Abort the compilation on error
 	 return 100;
     }

 // Code this word if it is not in an extended zone
 UDict.Mem[CodePosition]=pos;

 // Increase counter
 CodePosition++;

 return 0;
 }


// Codes a number inside current program
void programCodeNumber(int32_t value)
 {
 // Check if we can store in one byte
 if ((value>=MIN_1B_INT)&&(value<=MAX_1B_INT))
        {
	    if (int8Code((int8_t)value))
	        {
	        consoleErrorMessage(&MainContext,"Out of memory");
	        abortCompile();  // Abort the compilation on error
	        }
        return;
        }

 // Check if we can store in two bytes
 if ((value>=MIN_2B_INT)&&(value<=MAX_2B_INT))
        {
	    if (int16Code((int16_t)value))
	            {
	    	    consoleErrorMessage(&MainContext,"Out of memory");
	    	    abortCompile();  // Abort the compilation on error
	            }
        return;
        }

 // Last option is to encode as a four byte number
 if (int32Code(value))
        {
	    consoleErrorMessage(&MainContext,"Out of memory");
	    abortCompile();  // Abort the compilation on error
        }

 }

// Codes user program start position
// Returns 0 if it words ok
// Returns 1 in case of out of memory
int32_t codeUserPosition(uint16_t position)
 {
 uint16_t *pointer;

 // Check is there is space
 if ((CodePosition+2)>=UD_MEMSIZE) return 2;

 // Code the user word command
 baseCode("USERWORD");

 // Set pointer
 pointer=(uint16_t*)(UDict.Mem+CodePosition);

 // Set value
 (*pointer)=position;

 // Increase counter
 CodePosition+=2;

 return 0;
 }

// Returns the available user memory
// Uses different information in interactive and compile mode
int32_t  getUserMemory(void)
 {
 int32_t available;

 if (STATUS)
	 available=UD_MEMSIZE-CodePosition;
    else
     available=UD_MEMSIZE-UDict.Base.nextPos;

 return available;
 }

// Codes a print string ."
void codePrintString(char *pointer)
 {
 int32_t len,i;

 len=strLen(pointer);

 // Check if there is enough space
 if (getUserMemory()<(len+2))
       {
       consoleErrorMessage(&MainContext,"Out of memory");
       abortCompile();  // Abort the compilation on error
       return;
       }

 baseCode("P_STRING");

 for(i=0;i<=len;i++)
	 UDict.Mem[CodePosition++]=*(pointer+i);
 }

// Codes a normal counted string S"
void codeString(char *pointer)
 {
 int32_t len,i;

 len=strLen(pointer);

 // Check if there is enough space
 if (getUserMemory()<(len+2))
       {
       consoleErrorMessage(&MainContext,"Out of memory");
       abortCompile();  // Abort the compilation on error
       return;
       }

 baseCode("S_STRING");

 // Code length
 UDict.Mem[CodePosition++]=(uint8_t)len;

 // Code the rest of the string
 for(i=0;i<len;i++)
 	 UDict.Mem[CodePosition++]=*(pointer+i);
 }

/****************** PUBLIC EXECUTE AND INTERACTIVE FUNCTIONS **********************/

// Routines to be called before execution of
// a word from interactive mode
void executePrologue(ContextType *context)
 {
 // Erase return stack
 RstackInit(context);

 // Erase flags
 // They should be cleared anyway
 (context->Flags)&=(~(CFLAG_EXIT|CFLAG_ABORT));
 }

// Sets start word [INTERACTIVE DIRECTIVE WORD]
int32_t SetStartWord(ContextType *context,int32_t value)
 {
 UNUSED(value);
 UNUSED(context);

 char *name;
 uint16_t pos;

 // Get word to set
 name=tokenGet();

 // Check if we say "NOWORD"
 if (!strCaseCmp(name,"NOWORD"))
       {
	   UDict.Base.startWord=NO_WORD;
	   return 0;
       }

 // Locate this user word
 pos=locateUserWord(name);

 // Error if not found
 if (pos==NO_WORD)
     {
	 consoleErrorMessage(&MainContext,"Word not found");
	 return 0;
     }

 // Set as start word
 UDict.Base.startWord=pos;

 return 0;
 }

// Execution core used for programExecute and executeUserWord
// Defined inline to optimize speed and minimize stack usage
inline void wordExecutionCore(ContextType *context,uint16_t position)
 {
 uint16_t oldCounter,oldFrame;
 uint8_t byte;

 // Save old counter
 oldCounter=context->Counter;

 // Set new counter
 (context->Counter)=position;

 // Save old frame
 oldFrame=context->rstack.Frame;

 // Set new frame
 context->rstack.Frame=context->rstack.Pointer;

 // Execute all words while content is not zero
 // and we have not an exit or abort in progress
 while ((byte=UDict.Mem[(context->Counter)++])
	 		 &&(!((context->Flags)&CFLAGS_ENDWORD))
	 		 &&(!PORT_ABORT))
	          {
		      (BaseDictionary[byte].function)(context,BaseDictionary[byte].argument);
		      }

 // Check if we ended with PORT_ABORT
 if (PORT_ABORT) portAbort(context);

 // Restore Return stack frame
 context->rstack.Frame=oldFrame;

 // Restore old counter
 (context->Counter)=oldCounter;

 // If we are aborting do backtrace
 if ((context->Flags)&CFLAG_ABORT)
		          backtracePosition(context,position);

 // Erase exit flag for this word
 (context->Flags)&=(~CFLAG_EXIT);
 }

// Execute UDict program at the given position
// Called from interactive mode
// If primary is true ABORT will be erased on exit
//
// If we modify this function probably the same changes
// should be made to executeUserWord
void programExecute(ContextType *context,uint16_t position,int32_t primary)
 {
 // If run from interactive...
 if (primary) executePrologue(context);

 // Run execution core
 wordExecutionCore(context,position);

 if (primary)
     // Erase abort flag
     (context->Flags)&=(~CFLAG_ABORT);
 }


/***************** COMMAND FUNCTIONS *************************/

// Implements the ' directive   [INTERACTIVE]
int32_t findUserWord(ContextType *context,int32_t value)
 {
 UNUSED(value);

 char *name;
 uint16_t pos;

 // Get word to dump
 name=tokenGet();

 // Locate this user word
 pos=locateUserWord(name);

 // Error if not found
 if (pos==NO_WORD)
     {
	 consoleErrorMessage(&MainContext,"Word not found");
	 return 0;
     }

 // Put word position on the stack
 PstackPush(context,pos);

 return 0;
 }

// Dumps a user word  [INTERACTIVE]
int32_t UserWordDump(ContextType *context,int32_t value)
 {
 UNUSED(value);

 char *name;
 uint16_t pos,nsize,start,wsize;

 // Get word to dump
 name=tokenGet();

 // Locate this user word
 pos=locateUserWord(name);

 // Error if not found
 if (pos==NO_WORD)
     {
	 consoleErrorMessage(&MainContext,"Word not found");
	 return 0;
     }

 // Start of this word
 nsize=UDict.Mem[pos-3];
 start=pos-3-nsize;

 // End of this word
 if (previousWord==NO_WORD)
         wsize=UDict.Base.nextPos;
       else
    	 wsize=previousWord-3-UDict.Mem[previousWord-3];

 // Size to show
 wsize-=start;

 // Check if we are allowed to show responses
 if (NO_RESPONSE(context)) return 0;

 consolePrintf("%s%sWord position : %d%s",BREAK,BREAK,start,BREAK);
 consolePrintf("Word size : %d%s",wsize,BREAK);

 debugDumpMemory(UDict.Mem+start,start,wsize);

 return 0;
 }

// Allocates a new word name in the dictionary
// The name is obtained from the console
//
// Sets the CodePosition variable instead of
// the internal UDict.Base.nextPos in order to
// be able to undo the new word definition
//
// Returns 0 if ok
static int32_t newWordKernel(ContextType *context)
 {
 char *name;
 int32_t pos,nameLen;
 uint16_t *upointer;

 // Load word name
 name=tokenGet();

 // Check length of token
 if (checkTokenLength(name)) return 3;

 // Check that no word is in curse
 if (EditWord!=NO_WORD)
     {
	 consoleErrorMessage(context,"Already defining a word");
	 return 3;
     }

 // Check if it is a defined word
 pos=searchRegister((DictionaryEntry*) BaseDictionary,name);
 if (pos>=0)
   if (!((BaseDictionary[pos].flags)&DF_NCOMPILE))
	   consoleWarnMessage(context,"Redefining a Base Dictionary entry");

 pos=searchRegister((DictionaryEntry*) InteractiveDictionary,name);
 if (pos>=0)
     {
	 consoleErrorMessage(context,"Cannot redefine an Interactive Dictionary entry");
 	 return 1;
     }

 pos=searchRegister((DictionaryEntry*) GeneratorDictionary,name);
 if (pos>=0)
	 consoleWarnMessage(context,"Redefining a Generator Dictionary entry");

 // Check if already on user dictionary
 pos=locateUserWord(name);
 if (pos!=NO_WORD)
	 consoleWarnMessage(context,"Redefining an User Dictionary entry");

 // Check if there is enough space
 nameLen=strLen(name);
 if ((nameLen+4)>(UD_MEMSIZE-UDict.Base.nextPos))
     {
	 consoleErrorMessage(context,"Not enough space to define the new word");
	 return 2;
     }

 // Implement the new word

 // Position to start coding
 CodePosition=UDict.Base.nextPos;

 // Copy the word name changing to uppercase
 strCaseCpy(name,(char*)&(UDict.Mem[CodePosition]));

 // Write the string length
 CodePosition+=nameLen;
 UDict.Mem[CodePosition]=nameLen;

 // Write the word link
 CodePosition++;
 upointer=(uint16_t*)&(UDict.Mem[CodePosition]);
 (*upointer)=UDict.Base.lastWord;

 // Start of coding area
 CodePosition+=2;

 // Set this as the edit word
 EditWord=CodePosition;

 return 0;  // OK
 }


// Generates a new word using the ":" word
// Returns 0 if it works ok
//         non zero in case of error
int32_t GenNewWord(ContextType *context,int32_t value)
 {
 UNUSED(value);

 // Start the word definition from a word name from the console
 if (newWordKernel(context)) return 0;

 // Go to compile mode
 STATUS=1;

 // Init local var count
 nextLocal=0;
 // Init local mode
 localMode=NO_LOCAL;

 // Erase both stacks
 PstackInit(&MainContext);
 RstackInit(&MainContext);

 // Set this line as first one if we are not in file mode
 if (!(MainFlags&MFLAG_FILE)) CompileLine=1;

 return 0;
 }

// Ends a new word using the ";" word
int32_t EndNewWord(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int32_t ret;

 // Check if we are compiling a new word
 if (EditWord==NO_WORD)
    {
	consoleErrorMessage(context,"Cannot use ; outside of word definitions");
	return 0;
    }

 ret=baseCode("ENDWORD");
 if (ret==2)
     {
	 consoleErrorMessage(context,"Not enough space to end the word");
	 abortCompile();  // Abort the compilation on error
	 return 0;
     }

 // Check that both stacks are empty
 if ((RstackGetSize(&MainContext))||PstackGetSize(&MainContext))
         {
	     consoleErrorMessage(context,"Branch inconsistency in word");
	     abortCompile();
	     return 0;
         }

 // End the word by making the changes to UDict
 UDict.Base.lastWord=EditWord;
 UDict.Base.nextPos=CodePosition;

 // Go out of compile mode
 STATUS=0;
 EditWord=NO_WORD;

 // Erase both stacks
 PstackInit(&MainContext);
 RstackInit(&MainContext);

 return 0;
 }

// Ends a new word using without adding the end command
// Used for variables and create
int32_t EndNewWordWithoutEnd(void)
 {
 // End the word by making the changes to UDict
 UDict.Base.lastWord=EditWord;
 UDict.Base.nextPos=CodePosition;

 // Go out of compile mode
 STATUS=0;
 EditWord=NO_WORD;

 return 0;
 }

// Code a constant value   [INTERACTIVE]
int32_t CodeConstant(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int32_t val;

 // Get value from the stack
 if (PstackPop(context,&val)) return 0;

 // Get word name and compile
 if (GenNewWord(context,0)) return 0;

 // Check if there is enough space
 if (getUserMemory()<6)
      {
      consoleErrorMessage(context,"Out of memory");
      abortCompile();  // Abort the compilation on error
      return 0;
      }

 // Code the constant value
 programCodeNumber(val);

 // End the compilation
 EndNewWord(context,0);

 return 0;
 };

// Variable coding --------------------------------------------------------------

// Code a 32 bit variable
int32_t CodeVariable(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int32_t *pointer;

 // Get word name and compile
 if (GenNewWord(context,0)) return 0;

 // Check if there is enough space
 if (getUserMemory()<5)
      {
      consoleErrorMessage(context,"Out of memory");
      abortCompile();  // Abort the compilation on error
      return 0;
      }

 // Code the hidden variable code
 baseCode("VAR");

 // Set pointer for start value
 pointer=(int32_t*)(UDict.Mem+CodePosition);

 // Set zero value
 (*pointer)=0;

 // Increase counter
 CodePosition+=4;

 // End the compilation
 EndNewWordWithoutEnd();

 return 0;
 };

// Code a 16 bit variable
int32_t CodeHVariable(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int16_t *pointer;

 // Get word name and compile
 if (GenNewWord(context,0)) return 0;

 // Check if there is enough space
 if (getUserMemory()<3)
      {
      consoleErrorMessage(context,"Out of memory");
      abortCompile();  // Abort the compilation on error
      return 0;
      }

 // Code the hidden variable code
 baseCode("VARH");

 // Set pointer for start value
 pointer=(int16_t*)(UDict.Mem+CodePosition);

 // Set zero value
 (*pointer)=0;

 // Increase counter
 CodePosition+=2;

 // End the compilation
 EndNewWordWithoutEnd();

 return 0;
 };

// Code a 8 bit variable
int32_t CodeCVariable(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int8_t *pointer;

 // Get word name and compile
 if (GenNewWord(context,0)) return 0;

 // Check if there is enough space
 if (getUserMemory()<2)
      {
      consoleErrorMessage(context,"Out of memory");
      abortCompile();  // Abort the compilation on error
      return 0;
      }

 // Code the hidden variable code
 baseCode("VARC");

 // Set pointer for start value
 pointer=(int8_t*)(UDict.Mem+CodePosition);

 // Set zero value
 (*pointer)=0;

 // Increase counter
 CodePosition+=1;

 // End the compilation
 EndNewWordWithoutEnd();

 return 0;
 };

// Value Coding --------------------------------------------------------

// Code a 32 bit value
int32_t CodeValue(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int32_t *pointer;
 int32_t data;

 // Get data from the stack
 if (PstackPop(context,&data)) return 0;

 // Get word name and compile
 if (GenNewWord(context,0)) return 0;

 // Check if there is enough space
 if (getUserMemory()<5)
      {
      consoleErrorMessage(context,"Out of memory");
      abortCompile();  // Abort the compilation on error
      return 0;
      }

 // Code the hidden variable code
 baseCode("VAL");

 // Set pointer for start value
 pointer=(int32_t*)(UDict.Mem+CodePosition);

 // Set start value
 (*pointer)=data;

 // Increase counter
 CodePosition+=4;

 // End the compilation
 EndNewWordWithoutEnd();

 return 0;
 };

// Code a 16 bit value
int32_t CodeHValue(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int16_t *pointer;
 int32_t data;

 // Get data from the stack
 if (PstackPop(context,&data)) return 0;

 // Get word name and compile
 if (GenNewWord(context,0)) return 0;

 // Check if there is enough space
 if (getUserMemory()<3)
      {
      consoleErrorMessage(context,"Out of memory");
      abortCompile();  // Abort the compilation on error
      return 0;
      }

 // Code the hidden variable code
 baseCode("VALH");

 // Set pointer for start value
 pointer=(int16_t*)(UDict.Mem+CodePosition);

 // Set start value
 (*pointer)=(int16_t)data;

 // Increase counter
 CodePosition+=2;

 // End the compilation
 EndNewWordWithoutEnd();

 return 0;
 };

// Code a 8 bit value
int32_t CodeCValue(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int8_t *pointer;
 int32_t data;

 // Get data from the stack
 if (PstackPop(context,&data)) return 0;

 // Get word name and compile
 if (GenNewWord(context,0)) return 0;

 // Check if there is enough space
 if (getUserMemory()<2)
      {
      consoleErrorMessage(context,"Out of memory");
      abortCompile();  // Abort the compilation on error
      return 0;
      }

 // Code the hidden variable code
 baseCode("VALC");

 // Set pointer for start value
 pointer=(int8_t*)(UDict.Mem+CodePosition);

 // Set start value
 (*pointer)=(int8_t)data;

 // Increase counter
 CodePosition+=1;

 // End the compilation
 EndNewWordWithoutEnd();

 return 0;
 };

// ---------------------------------------------------------------------

// Forgets an user word an all above it   [INTERACTIVE]
int32_t UserWordForget(ContextType *context,int32_t value)
 {
 UNUSED(value);

 char *name;
 uint16_t pos,nsize,start,i;

 if (anythingBackground())
       {
	   consoleErrorMessage(context,"Cannot forget with running background processes");
	   return 0;
       }

 if (isAnyCallback())
      {
 	  consoleErrorMessage(context,"Cannot forget with registered callbacks");
 	  return 0;
      }

 // Get word to dump
 name=tokenGet();

 // Locate this user word
 pos=locateUserWord(name);

 // Error if not found
 if (pos==NO_WORD)
     {
	 consoleErrorMessage(context,"Word not found");
	 return 0;
     }

 // Locate word start
 nsize=UDict.Mem[pos-3];
 start=pos-3-nsize;

 // Erase memory
 for(i=start;i<UDict.Base.nextPos;i++)
	                     UDict.Mem[i]=0xFF;

 // Set last word after forget
 UDict.Base.lastWord=searchWord;

 // Set first free position
 UDict.Base.nextPos=start;

 // Erase start word if needed
 if (UDict.Base.startWord!=NO_WORD)  // If there is a start word...
	 if (UDict.Base.startWord>=pos)
	      {
          UDict.Base.startWord=NO_WORD;
          if (SHOW_INFO(context))
              { consolePrintf("Start word reference has been eliminated%s",BREAK); }
	      }

 return 0;
 }

// Decode a int8t value
int32_t int8decode(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int8_t *pointer;
 int32_t val;

 // Set pointer
 pointer=(int8_t*)(UDict.Mem+context->Counter);

 // Push on the stack
 val=(int32_t)(*pointer);
 PstackPush(context,val);

 // Increase counter
 (context->Counter)++;

 return 0;
 }

// Decode a int16t value
int32_t int16decode(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int16_t *pointer;
 int32_t val;

 // Set pointer
 pointer=(int16_t*)(UDict.Mem+context->Counter);

 // Push on the stack
 val=(int32_t)(*pointer);
 PstackPush(context,val);

 // Increase counter
 (context->Counter)+=2;

 return 0;
 }

// Decode a int32t value
int32_t int32decode(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int32_t *pointer;

 // Set pointer
 pointer=(int32_t*)(UDict.Mem+context->Counter);

 // Push on the stack
 PstackPush(context,(*pointer));

 // Increase counter
 (context->Counter)+=4;

 return 0;
 }

// Execute a user function from address in current run position
// Called from another word
// Uses the same execution core that programExecute
int32_t executeUserWord(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int16_t *pointer;

 // Set pointer to current position
 pointer=(int16_t*)(UDict.Mem+context->Counter);

 // Increase counter before saving it
 (context->Counter)+=2;

 // Run execution core
 wordExecutionCore(context,*pointer);

 return 0;
 }

/*
// Execute a user function from address in current run position
// Called from another word
// If we modify this function probably the same changes
// should be made to programExecute
int32_t executeUserWord(ContextType *context,int32_t value)
 {
 UNUSED(value);

 uint16_t oldCounter;
 uint8_t byte;
 int16_t *pointer;

 // Set pointer to current position
 pointer=(int16_t*)(UDict.Mem+context->Counter);

 // Save old counter increased by 2
 oldCounter=(context->Counter)+2;

 // Set new counter
 (context->Counter)=(*pointer);

 // Set Return stack frame
 (context->rstack.Frame)=context->rstack.Pointer;

 // Execute all words while content is not zero
 // and we have not an exit or abort in progress
 while ((byte=UDict.Mem[(context->Counter)++])
		 &&(!((context->Flags)&CFLAGS_ENDWORD))
		 &&(!PORT_ABORT))
    {
	(BaseDictionary[byte].function)(context,BaseDictionary[byte].argument);
	}

 // Check if we ended with PORT_ABORT
 if (PORT_ABORT) portAbort(context);

 // Restore old counter
(context->Counter)=oldCounter;

 // Restore Return stack frame
 (context->rstack.Pointer)=context->rstack.Frame;

 // If we are aborting
 if ((context->Flags)&CFLAG_ABORT)
	 backtracePosition(context,(int32_t)(*pointer));

 // Erase exit flag for this word
 // Don't erase ABORT flag
 (context->Flags)&=(~CFLAG_EXIT);

 return 0;
 }
*/


// Return the variable position
int32_t executeVariable(ContextType *context,int32_t value)
 {
 UNUSED(value);

 uint32_t position;
 int32_t *ipos;

 // Take current run position as variable location
 position=(uint32_t)(UDict.Mem+context->Counter);

 // Set pointer
 ipos=(int32_t*)&position;

 // Push this position
 PstackPush(context,*ipos);

 // Set exit flag
 (context->Flags)|=CFLAG_EXIT;

 return 0;
 }

// Value execution functions ---------------------------------------

// Return the 32bit value
int32_t executeValue(ContextType *context,int32_t value)
 {
 UNUSED(value);

 uint32_t data;

 // Take current run position as variable location
 data=*(uint32_t*)(UDict.Mem+context->Counter);

 // Push this data
 PstackPush(context,data);

 // Set exit flag
 (context->Flags)|=CFLAG_EXIT;

 return 0;
 }

// Return the 16bit value
int32_t executeHValue(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int16_t data;

 // Take current run position as variable location
 data=*(uint16_t*)(UDict.Mem+context->Counter);

 // Push this data
 PstackPush(context,(int32_t)data);

 // Set exit flag
 (context->Flags)|=CFLAG_EXIT;

 return 0;
 }

// Return the 8bit value
int32_t executeCValue(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int8_t data;

 // Take current run position as variable location
 data=*(uint8_t*)(UDict.Mem+context->Counter);

 // Push this data
 PstackPush(context,(int32_t)data);

 // Set exit flag
 (context->Flags)|=CFLAG_EXIT;

 return 0;
 }

// Variable recall functions ---------------------------------------

// int32_t Variable recall
int32_t executeVariableRecall(ContextType *context,int32_t value)
 {
 UNUSED(value);

 uint32_t pos;
 int32_t *pointer;

 // Get direction from stack
 if (PstackPop(context,(int32_t*)&pos)) return 0;

 // Set pointer
 pointer=(int32_t*)pos;

 // Recall data
 PstackPush(context,*pointer);

 return 0;
 }

// int16_t Variable recall
int32_t executeHVariableRecall(ContextType *context,int32_t value)
 {
 UNUSED(value);

 uint32_t pos;
 int16_t *pointer;

 // Get direction from stack
 if (PstackPop(context,(int32_t*)&pos)) return 0;

 // Set pointer
 pointer=(int16_t*)pos;

 // Recall data
 PstackPush(context,(int32_t)*pointer);

 return 0;
 }



// int8_t Variable recall
int32_t executeCVariableRecall(ContextType *context,int32_t value)
 {
 UNUSED(value);

 uint32_t pos;
 int8_t *pointer;

 // Get direction from stack
 if (PstackPop(context,(int32_t*)&pos)) return 0;

 // Set pointer
 pointer=(int8_t*)pos;

 // Recall data
 PstackPush(context,(int32_t)*pointer);

 return 0;
 }

// Intelligent Variable recall
// Detects the kind of variable
int32_t executeIntelligentVariableRecall(ContextType *context,int32_t value)
 {
 UNUSED(value);

 uint32_t pos;
 int32_t *pointer32;
 int16_t *pointer16;
 int8_t  *pointer8;
 uint8_t *p8;

 // Get direction from stack
 if (PstackPop(context,(int32_t*)&pos)) return 0;

 // Set pointers
 pointer32=(int32_t*)pos;
 pointer16=(int16_t*)pos;
 pointer8=(int8_t*)pos;

 // Set pointer to kind of variable
 p8=(uint8_t*)pointer32;
 p8--;

 switch (*p8)
   {
   case VAR_CODE:
	   PstackPush(context,*pointer32);
	   break;
   case VARH_CODE:
	   PstackPush(context,(int32_t)*pointer16);
	   break;
   case VARC_CODE:
	   PstackPush(context,(int32_t)*pointer8);
	   break;
   }

 return 0;
 }

// int32_t Variable store
int32_t executeVariableStore(ContextType *context,int32_t value)
 {
 UNUSED(value);

 uint32_t pos;
 int32_t *pointer,val;

 // Get direction from stack
 if (PstackPop(context,(int32_t*)&pos)) return 0;

 // Get val from stack
 if (PstackPop(context,&val)) return 0;

 // Set pointer
 pointer=(int32_t*)pos;

 // Store data
 (*pointer)=val;

 return 0;
 }

// int16_t Variable store
int32_t executeHVariableStore(ContextType *context,int32_t value)
 {
 UNUSED(value);

 uint32_t pos;
 int32_t val;
 int16_t *pointer;

 // Get direction from stack
 if (PstackPop(context,(int32_t*)&pos)) return 0;

 // Get val from stack
 if (PstackPop(context,&val)) return 0;

 // Check val range  (Removed)
 //if (val<MIN_2B_INT) val=MIN_2B_INT;
 //if (val>MAX_2B_INT) val=MAX_2B_INT;

 // Set pointer
 pointer=(int16_t*)pos;

 // Store data
 (*pointer)=(int16_t)val;

 return 0;
 }

// int8_t Variable store
int32_t executeCVariableStore(ContextType *context,int32_t value)
 {
 UNUSED(value);

 uint32_t pos;
 int32_t val;
 int8_t *pointer;

 // Get direction from stack
 if (PstackPop(context,(int32_t*)&pos)) return 0;

 // Get val from stack
 if (PstackPop(context,&val)) return 0;

 // Check val range (Removed)
 //if (val<MIN_1B_INT) val=MIN_1B_INT;
 //if (val>MAX_1B_INT) val=MAX_1B_INT;

 // Set pointer
 pointer=(int8_t*)pos;

 // Store data
 (*pointer)=(int8_t)val;

 return 0;
 }

// Intelligent Variable store
// Detects the kind of variable
int32_t executeIntelligentVariableStore(ContextType *context,int32_t value)
 {
 UNUSED(value);

 uint32_t pos;
 int32_t *pointer32;
 int16_t *pointer16;
 int8_t  *pointer8;
 uint8_t *p8;
 int32_t val;

 // Get direction from stack
 if (PstackPop(context,(int32_t*)&pos)) return 0;

 // Set pointers
 pointer32=(int32_t*)pos;
 pointer16=(int16_t*)pos;
 pointer8=(int8_t*)pos;

 // Set pointer to kind of variable
 p8=(uint8_t*)pointer32;
 p8--;

 // Get val from stack
 if (PstackPop(context,&val)) return 0;

 switch (*p8)
   {
   case VAR_CODE:
	   (*pointer32)=val;
	   break;
   case VARH_CODE:
	   // Check val range (Removed)
	   //if (val<MIN_2B_INT) val=MIN_2B_INT;
	   //if (val>MAX_2B_INT) val=MAX_2B_INT;
	   // Store data
	   (*pointer16)=(int16_t)val;
	   break;
   case VARC_CODE:
	   // Check val range (Removed)
	   //if (val<MIN_1B_INT) val=MIN_1B_INT;
	   //if (val>MAX_1B_INT) val=MAX_1B_INT;
	   // Store data
	   (*pointer8)=(int8_t)val;
	   break;
   }

 return 0;
 }

//---------VARIABLE STORE AND ADD --------------------

// int32_t Variable add and store
int32_t executeVariableStoreAdd(ContextType *context,int32_t value)
 {
 UNUSED(value);

 uint32_t pos;
 int32_t *pointer,val;

 // Get direction from stack
 if (PstackPop(context,(int32_t*)&pos)) return 0;

 // Get val from stack
 if (PstackPop(context,&val)) return 0;

 // Set pointer
 pointer=(int32_t*)pos;

 // Store and add data
 (*pointer)+=val;

 return 0;
 }

// int16_t Variable store and add
int32_t executeHVariableStoreAdd(ContextType *context,int32_t value)
 {
 UNUSED(value);

 uint32_t pos;
 int32_t val;
 int16_t *pointer;

 // Get direction from stack
 if (PstackPop(context,(int32_t*)&pos)) return 0;

 // Get val from stack
 if (PstackPop(context,&val)) return 0;

 // Set pointer
 pointer=(int16_t*)pos;

 // Store data
 (*pointer)+=(int16_t)val;

 return 0;
 }

// int8_t Variable store and Add
int32_t executeCVariableStoreAdd(ContextType *context,int32_t value)
 {
 UNUSED(value);

 uint32_t pos;
 int32_t val;
 int8_t *pointer;

 // Get direction from stack
 if (PstackPop(context,(int32_t*)&pos)) return 0;

 // Get val from stack
 if (PstackPop(context,&val)) return 0;

 // Set pointer
 pointer=(int8_t*)pos;

 // Store data
 (*pointer)+=(int8_t)val;

 return 0;
 }

// Intelligent Variable store and Add
// Detects the kind of variable
int32_t executeIntelligentVariableStoreAdd(ContextType *context,int32_t value)
 {
 UNUSED(value);

 uint32_t pos;
 int32_t *pointer32;
 int16_t *pointer16;
 int8_t  *pointer8;
 uint8_t *p8;
 int32_t val;

 // Get direction from stack
 if (PstackPop(context,(int32_t*)&pos)) return 0;

 // Set pointers
 pointer32=(int32_t*)pos;
 pointer16=(int16_t*)pos;
 pointer8=(int8_t*)pos;

 // Set pointer to kind of variable
 p8=(uint8_t*)pointer32;
 p8--;

 // Get val from stack
 if (PstackPop(context,&val)) return 0;

 switch (*p8)
   {
   case VAR_CODE:
	   (*pointer32)+=val;
	   break;
   case VARH_CODE:
	   // Store data and add
	   (*pointer16)+=(int16_t)val;
	   break;
   case VARC_CODE:
	   // Store data and add
	   (*pointer8)+=(int8_t)val;
	   break;
   }

 return 0;
 }

int32_t wordHelpCommand(ContextType *context,int32_t value)
 {
 UNUSED(value);

 char *name;
 uint16_t upos;
 int32_t pos,found=0;

 // Check if we are able to show info
 if (NO_INFO(context)) return 0;

 // Get word to dump
 name=tokenGet();

 // Search in interactive dictionary
 pos=searchRegister((DictionaryEntry*) InteractiveDictionary,name);
 if (pos>=0)
     {
	 consolePrintf("Found in Interactive dictionary:%s",BREAK);
	 wordHelp((DictionaryEntry*)(InteractiveDictionary+pos));
	 found=1;
     }

 // Locate this user word
 upos=locateUserWord(name);
 if (upos!=NO_WORD)
    {
	found=1;
	consolePrintf("Found in User dictionary%s",BREAK);
	consolePrintf("Help is not available here%s%s",BREAK,BREAK);
    }

 // Search in generator dictionary
 pos=searchRegister((DictionaryEntry*) GeneratorDictionary,name);
 if (pos>=0)
     {
	 consolePrintf("Found in Generator dictionary:%s",BREAK);
	 wordHelp((DictionaryEntry*)(GeneratorDictionary+pos));
	 found=1;
     }

 // Search in interactive dictionary
 pos=searchRegister((DictionaryEntry*) BaseDictionary,name);
 if (pos>=0)
   if (!(BaseDictionary[pos].flags&DF_NCOMPILE))
        {
	    consolePrintf("Found in Base dictionary:%s",BREAK);
	    wordHelp((DictionaryEntry*)(BaseDictionary+pos));
	    found=1;
        }

 if (!found)
	 consolePrintf("Word not found%s",BREAK);

 CBK;  // Last line break

 return 0;
 }


// List all words and descriptions from
// built-in dictionaries
int32_t baseWords(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int32_t pos;

 // Check if we are able to show info
 if (NO_INFO(context)) return 0;

 // Search in interactive dictionary
 consolePrintf("%sInteractive dictionary:%s",BREAK,BREAK);
 pos=-1;
 while (InteractiveDictionary[++pos].function!=NULL)
    wordHelp((DictionaryEntry*)(InteractiveDictionary+pos));

 // Search in generator dictionary
 consolePrintf("%sGenerator dictionary:%s",BREAK,BREAK);
 pos=-1;
 while (GeneratorDictionary[++pos].function!=NULL)
    wordHelp((DictionaryEntry*)(GeneratorDictionary+pos));

 // Search in base dictionary
 consolePrintf("%sBase dictionary:%s",BREAK,BREAK);
 pos=-1;
 while (BaseDictionary[++pos].function!=NULL)
	if (!(InteractiveDictionary[pos].flags&DF_NCOMPILE))
       wordHelp((DictionaryEntry*)(BaseDictionary+pos));

 CBK;  // Last line break

 return 0;
 }


// TO Functions ------------------------------------------

// Immediate TO function                 [DIRECTIVE]
// Implements TO  and +TO functionalities
// We use the value to identify if working with norma or add mode
int32_t inmediateTO(ContextType *context,int32_t value)
 {
 char *name;
 int32_t pos,code,data,i;

 // Get word to dump
 name=tokenGet();

 // Check if it is a local
 if (STATUS) // If we are in compile mode..
	 if (nextLocal) //..and there are locals
		 for(i=0;i<nextLocal;i++)
			 if (!strCaseCmp(name,LocalNames[i]))
			     {
				 // Found
				 if (value==IT_NORMAL)
				         // Code the Set RStack Hidden Word
				         baseCode("SETR");
				       else
				    	 // Code the Add to RStack Hidden Word
				    	 baseCode("ADDR");

				 //DEBUG_STRING("Code Set Local: ",name);

				 // Set the RStack position relative to current fame
				 UDict.Mem[CodePosition++]=LocalIndex[i];

				 return 0;
			     }

 // Locate this user word
 pos=locateUserWord(name);

 // Error if not found
 if (pos==NO_WORD)
     {
	 consolePrintf("ERROR: Value [%s] word not found",name);
	 errorEpilogue();
	 return 0;
     }

 // Check if the word is a value
 code=UDict.Mem[pos++];
 if ((code!=VAL_CODE)&&(code!=VALH_CODE)&&(code!=VALC_CODE))
       {
	   consoleErrorMessage(context,"Illegal TO/+TO destination");
	   return 0;
       }

 // If we are in interactive mode
 if (!STATUS)
    {
	// Pop data from the stack
	if (PstackPop(context,&data)) return 0;

	switch (code)
	   {
	   case  VAL_CODE:
		   if (value==IT_NORMAL)
		            *((int32_t*)(UDict.Mem+pos))=data;  // TO32
		           else
		        	*((int32_t*)(UDict.Mem+pos))+=data; // +TO32
		   break;
	   case  VALH_CODE:
		   if (value==IT_NORMAL)
		            *((int16_t*)(UDict.Mem+pos))=(int16_t)data;   // TO16
		           else
		        	*((int16_t*)(UDict.Mem+pos))+=(int16_t)data;  // +TO16
		   break;
	   case  VALC_CODE:
		   if (value==IT_NORMAL)
		            *(UDict.Mem+pos)=(int8_t)data;  // TO8
		           else
		            *(UDict.Mem+pos)+=(int8_t)data; // +TO8
		   break;
	   }

	return 0;
    }

 // If we arrive here we should be in compile mode

 // Check if there is enough space
 if (getUserMemory()<4)
      {
      consoleErrorMessage(context,"Out of memory");
      abortCompile();  // Abort the compilation on error
      return 0;
      }

 // Compile the start word
 switch (code)
 	{
 	case VAL_CODE:
 		if (value==IT_NORMAL)
 		        baseCode("TOVAL");
 		       else
 		    	baseCode("ADDTOVAL");
 		break;
 	case VALH_CODE:
 		if (value==IT_NORMAL)
 		        baseCode("TOHVAL");
 		       else
 		    	baseCode("ADDTOHVAL");
 		break;
 	case VALC_CODE:
 		if (value==IT_NORMAL)
 		        baseCode("TOCVAL");
 		       else
 		    	baseCode("ADDTOCVAL");
 		break;
 	   }

 // Compile the position
 allocate16u((uint16_t)pos);

 return 0;
 }

// Stores TOS on a 32bit value
int32_t executeTOVAL(ContextType *context,int32_t value)
 {
 UNUSED(value);
 int32_t *data,dval;

 // Get pointer
 data=(int32_t*)(UDict.Mem+getAddrFromHere(context));

 // Get data from stack
 if (PstackPop(context,&dval)) return 0;

 // Set value
 *data=dval;

 return 0;
 }

// Stores TOS on a 16bit value
int32_t executeTOHVAL(ContextType *context,int32_t value)
 {
 UNUSED(value);
 int16_t *data;
 int32_t dval;

 // Get pointer
 data=(int16_t*)(UDict.Mem+getAddrFromHere(context));

 // Get data from stack
 if (PstackPop(context,&dval)) return 0;

 // Set value
 *data=(int16_t)dval;

 return 0;
 }

// Stores TOS on a 8bit value
int32_t executeTOCVAL(ContextType *context,int32_t value)
 {
 UNUSED(value);
 int8_t *data;
 int32_t dval;

 // Get pointer
 data=(int8_t*)(UDict.Mem+getAddrFromHere(context));

 // Get data from stack
 if (PstackPop(context,&dval)) return 0;

 // Set value
 *data=(int8_t)dval;

 return 0;
 }

// Adds TOS to a 32bit value
int32_t executeADDTOVAL(ContextType *context,int32_t value)
 {
 UNUSED(value);
 int32_t *data,dval;

 // Get pointer
 data=(int32_t*)(UDict.Mem+getAddrFromHere(context));

 // Get data from stack
 if (PstackPop(context,&dval)) return 0;

 // Set value
 *data+=dval;

 return 0;
 }

// Adds TOS to a 16bit value
int32_t executeADDTOHVAL(ContextType *context,int32_t value)
 {
 UNUSED(value);
 int16_t *data;
 int32_t dval;

 // Get pointer
 data=(int16_t*)(UDict.Mem+getAddrFromHere(context));

 // Get data from stack
 if (PstackPop(context,&dval)) return 0;

 // Set value
 *data+=(int16_t)dval;

 return 0;
 }

// Adds TOS to a 8bit value
int32_t executeADDTOCVAL(ContextType *context,int32_t value)
 {
 UNUSED(value);
 int8_t *data;
 int32_t dval;

 // Get pointer
 data=(int8_t*)(UDict.Mem+getAddrFromHere(context));

 // Get data from stack
 if (PstackPop(context,&dval)) return 0;

 // Set value
 *data+=(int8_t)dval;

 return 0;
 }


//--------------------------------------------------------

// Basic program function
// Defines runtime operation of basic program commands
int32_t ProgramFunction(ContextType *context,int32_t value)
 {
 uint16_t addr;
 int32_t pos;

 switch (value)
   {
   case PF_F_EXTEND1:  // Extend zone 1
       pos=UDict.Mem[(context->Counter)++]+EXT1_START;
       (BaseDictionary[pos].function)(context,BaseDictionary[pos].argument);
       break;

   case PF_F_EXTEND2:  // Extend zone 2
       pos=UDict.Mem[(context->Counter)++]+EXT2_START;
       (BaseDictionary[pos].function)(context,BaseDictionary[pos].argument);
       break;

   case PF_F_EXTEND3:  // Extend zone 3
       pos=UDict.Mem[(context->Counter)++]+EXT3_START;
       (BaseDictionary[pos].function)(context,BaseDictionary[pos].argument);
       break;

   case PF_F_EXIT:  // Exit from current word
	   ExitWord(context);
       break;

   case PF_F_ABORT:  // Abort to interactive mode
	   AbortExecution(context);
	   runtimeErrorMessage(context,"ABORT executed");
       break;

   case PF_F_EXECUTE_INT: // EXECUTE from interactive mode
	   // Try to pop one value
	   if (PstackPop(context,&pos)) return 0;

	   // Cast to int16
	   addr=(uint16_t)pos;

	   // Check range
	   if (addr>=UDict.Base.nextPos) return 0;

	   // Execute in primary mode
	   programExecute(context,addr,1);
	   break;

   case PF_F_EXECUTE_WORD: // EXECUTE from another word
	   // Try to pop one value
	   if (PstackPop(context,&pos)) return 0;

	   // Cast to int16
	   addr=(uint16_t)pos;

	   // Check range
	   if (addr>=UDict.Base.nextPos) return 0;

	   // Execute in secondary mode
	   programExecute(context,addr,0);
	   break;

   case PF_F_FORGETALL: // Erases all program memory data [INTERACTIVE]
       if (anythingBackground())
	         {
		     consoleErrorMessage(context,"Cannot erase with running background processes");
		     return 0;
	         }
       if (isAnyCallback())
            {
    	    consoleErrorMessage(context,"Cannot erase with registered callbacks");
    	   	return 0;
            }
       programErase();
	   break;

   case PF_F_SAVE: // Saves all program memory data [INTERACTIVE]
	   if (anythingBackground())
	         {
		     consoleErrorMessage(context,"Cannot save with running background processes");
		     return 0;
	         }
       if (isAnyCallback())
            {
    	    consoleErrorMessage(context,"Cannot save with registered callbacks");
    	   	return 0;
            }
	   if (saveUserDictionary())
		   consoleErrorMessage(context,"Cannot save User Dictionary");
	   break;

   case PF_F_LOAD: // Loads last saved dictionary [INTERACTIVE]
	   if (anythingBackground())
	         {
		     consoleErrorMessage(context,"Cannot load with running background processes");
		     return 0;
	         }
       if (isAnyCallback())
            {
    	    consoleErrorMessage(context,"Cannot load with registered callbacks");
    	   	return 0;
            }
	   if (loadUserDictionary())
		   consoleErrorMessage(context,"Cannot load User Dictionary");
	   break;

   case PF_F_HERE: // Shows next user position to compile on [INTERACTIVE]
	   if (EditWord==NO_WORD)  // If we are not editing any word
	         {
		     PstackPush(context,(int32_t)UDict.Base.nextPos);
		     return 0;
	         }
	   // If we arrive here we are editing a word
	   PstackPush(context,(int32_t)CodePosition);
	   break;

   case PF_F_FSTART:           // File start word
	   CompileLine=1;          // Set this line as first one
	   MainFlags|=MFLAG_FILE;  // Set the file flag
	   lastVerbose=MainContext.VerboseLevel;         // Backup verbose level
	   MainContext.VerboseLevel&=FILE_VERBOSE_MASK;  // Set verbose mask
	   break;

   case PF_F_FEND:   // File end word
	   MainFlags&=(~MFLAG_FILE);  // Clear the file flag
	   MainContext.VerboseLevel=lastVerbose; // Restore verbose level
   	   break;

   case PF_F_DEBUG_ON:   // Debug ON
	   MainFlags|=MFLAG_DEBUG_ON;
   	   break;

   case PF_F_DEBUG_OFF:   // Debug OFF
	   MainFlags&=(~MFLAG_DEBUG_ON);
   	   break;

   case PF_F_P_STRING: // Print string
	   while (UDict.Mem[context->Counter])
	           {
		       if (SHOW_RESPONSE(context))
	                 consolePutChar(UDict.Mem[(context->Counter)++]);
	           }
	   (context->Counter)++;
	   break;

   case PF_F_S_STRING: // String
	   // Push address of string
	   PstackPush(context,(int32_t)(UDict.Mem+(context->Counter)+1));
	   // Push count of string
	   PstackPush(context,(int32_t)UDict.Mem[context->Counter]);
	   // Increment counter to end of string
	   (context->Counter)+=UDict.Mem[context->Counter]+1;
	   break;

   case PF_F_WORD_EXCEPTION: // Use of a word that should not be compiled
	   runtimeErrorMessage(context,"Execute of illegal compile word exception");
	   return 0;
	   break;
   };

 return 0;
 }

// Basic generator function
// Defines the principal generator functions
int32_t GeneratorFunction(ContextType *context,int32_t value)
 {
 int32_t data;

 switch (value)
   {
   case GF_F_RECURSE:   // Call itself
	   codeUserPosition(EditWord);
       break;

   case GF_F_GO_INTERACTIVE:  // Implement "["
	   STATUS=0;
	   break;

   case GF_F_LITERAL:  // Codes stack top
	   if (PstackPop(context,&data)) return 0;
	   programCodeNumber(data);
	   break;

   case GF_F_ASRT_START: // Codes "assert("
	   if (MainFlags&(MFLAG_DEBUG_ZONE|MFLAG_ASSERT_ZONE))
	         {
		     consoleErrorMessage(context,"Nested assert zone");
		     return 0;
	         }
	   MainFlags|=MFLAG_ASSERT_ZONE;  // Start of assert zone
	   break;

   case GF_F_DEBUG_START: // Codes "debug("
	   if (MainFlags&(MFLAG_DEBUG_ZONE|MFLAG_ASSERT_ZONE))
	         {
		     consoleErrorMessage(context,"Nested debug zone");
		     return 0;
	         }
	   MainFlags|=MFLAG_DEBUG_ZONE;  // Start of debug zone
	   break;
   };

 return 0;
 }

// End of assert or debug in compilation
int32_t assertEnd(ContextType *context,int32_t value)
 {
 UNUSED(value);

 // Check if we are in a assert zone
 if (!(MainFlags&(MFLAG_ASSERT_ZONE|MFLAG_DEBUG_ZONE)))
       {
	   consoleErrorMessage(context,"Out of context \")\"");
	   return 0;
       }

 // If it was a debug zone just return after clearing the flag
 if (MainFlags&MFLAG_DEBUG_ZONE)
      {
	  MainFlags&=(~MFLAG_DEBUG_ZONE);
	  return 0;
      }

 // Deactivate the assert zone flag
 MainFlags&=(~MFLAG_ASSERT_ZONE);

 // If assert is active in current compilation...
 if (MainFlags&MFLAG_DEBUG_ON)
     {
	 // Try to compile the ASRT_CHECK command
	 if (baseCode("ASRT_CHECK"))
	        {
		    consoleErrorMessage(context,"Error compiling assertion");
		    return 0;
	        }

     }
 return 0;
 }

// Check in an assert zone   [HIDDEN]
int32_t assertCheck(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int32_t flag,code,ret;

 // Pop flag from stack
 ret=PstackPop(context,&code);

 // Pop code from stack
 if (!ret) ret=PstackPop(context,&flag);

 // Check errors
 if (ret)
    {
	runtimeErrorMessage(context,"Fail in assert parameters");
	return 0;
    }

 // Check assert
 if (!flag)
    {
	if (SHOW_ERROR(context))
	        consolePrintf("RUN ERROR: Assert failed with code %d%s",code,BREAK);
	// Aborts current run
	AbortExecution(context);
    }

 return 0;
 }

// Word disassemble -----------------------------------------------------

// Get a int8t value from memory
static int32_t int8get(void)
 {
 int8_t *pointer;

 // Set pointer
 pointer=(int8_t*)(UDict.Mem+decodePosition);

 // Increase pointer
 decodePosition++;

 return (int32_t)(*pointer);
 }

// Get a int16t value from memory
static int32_t int16get(void)
 {
 int16_t *pointer;

 // Set pointer
 pointer=(int16_t*)(UDict.Mem+decodePosition);

 // Increase counter
 decodePosition+=2;

 return (int32_t)(*pointer);
 }

// Get a int16t value from memory
static int32_t uint16get(void)
 {
 uint16_t *pointer;

 // Set pointer
 pointer=(uint16_t*)(UDict.Mem+decodePosition);

 // Increase counter
 decodePosition+=2;

 return (int32_t)(*pointer);
 }

// Get a int8t value from memory
static int32_t uint8get(void)
 {
 uint8_t *pointer;

 // Set pointer
 pointer=(uint8_t*)(UDict.Mem+decodePosition);

 // Increase counter
 decodePosition++;

 return (int32_t)(*pointer);
 }

// Get a int32t value from memory
static int32_t int32get(void)
 {
 int32_t *pointer;

 // Set pointer
 pointer=(int32_t*)(UDict.Mem+decodePosition);

 // Increase counter
 decodePosition+=4;

 return (*pointer);
 }

// Show a command by its number
static void showCommand(int32_t data)
 {
 int32_t value;

 consolePrintf("%s",BaseDictionary[data].name);

 // Check if an addr follows
 if (BaseDictionary[data].flags&DF_ADDR)
     {
	 value=uint16get();
	 consolePrintf(" %d",value);
     }

 // Check if a unsigned byte follows
 if (BaseDictionary[data].flags&DF_BYTE)
      {
 	  value=uint8get();
 	  consolePrintf(" %d",value);
      }

 CBK;
 }

// Views the code of a word      [INTERACTIVE]
int32_t See(ContextType *context,int32_t value)
 {
 UNUSED(value);

 char *name;
 uint8_t data;
 int32_t nword,number;

 // Check if info is enabled
 if (NO_INFO(context)) return 0;

 // Get word to dump
 name=tokenGet();

 // Locate this user word
 decodePosition=locateUserWord(name);

 // Error if not found
 if (decodePosition==NO_WORD)
     {
	 consoleErrorMessage(&MainContext,"Word not found");
	 return 0;
     }

 // Check that it is really a program
 data=UDict.Mem[decodePosition];

 if (data==VAR_CODE)
     { consolePrintf("This is a 32 bit variable%s",BREAK); return 0; }
 if (data==VARH_CODE)
     { consolePrintf("This is a 16 bit variable%s",BREAK); return 0; }
 if (data==VARC_CODE)
     { consolePrintf("This is a 8 bit variable%s",BREAK); return 0; }
 if (data==VAL_CODE)
     { consolePrintf("This is a 32 bit value%s",BREAK); return 0; }
 if (data==VALH_CODE)
     { consolePrintf("This is a 16 bit value%s",BREAK); return 0; }
 if (data==VALC_CODE)
     { consolePrintf("This is a 8 bit value%s",BREAK); return 0; }
 if (data==CRT_CODE)
     { consolePrintf("This is a create field%s",BREAK); return 0; }

 consolePrintf("Decoding of word %s%s%s",name,BREAK,BREAK);

 // Explore all the word code
 do
  {
  // Get data from this position
  data=UDict.Mem[decodePosition];

  // Write address
  consolePrintf("%8d : ",decodePosition);

  // Increase position
  decodePosition++;

  switch (data)
      {
      case EXT1_CODE:
    	  data=UDict.Mem[decodePosition++];
    	  nword=EXT1_START+data;
    	  showCommand(nword);
    	  break;

      case EXT2_CODE:
    	  data=UDict.Mem[decodePosition++];
    	  nword=EXT2_START+data;
    	  showCommand(nword);
    	  break;

      case EXT3_CODE:
    	  data=UDict.Mem[decodePosition++];
    	  nword=EXT2_START+data;
    	  showCommand(nword);
    	  break;

      case NUM1B_CODE:
    	  number=int8get();
    	  consolePrintf("%d%s",number,BREAK);
    	  break;

      case NUM2B_CODE:
    	  number=int16get();
    	  consolePrintf("%d%s",number,BREAK);
    	  break;

      case NUM4B_CODE:
    	  number=int32get();
    	  consolePrintf("%d%s",number,BREAK);
    	  break;

      case SS_CODE:
    	  consolePrintf("S\"");
    	  number=uint8get(); // Get the size
    	  for(;number>0;number--)
    		  consolePutChar(UDict.Mem[decodePosition++]);
    	  consolePrintf("\"%s",BREAK);
    	  break;

      case PS_CODE:
    	  consolePrintf(".\"");
    	  while (UDict.Mem[decodePosition]!=0)
    	          consolePutChar(UDict.Mem[decodePosition++]);
    	  decodePosition++;
    	  consolePrintf("\"%s",BREAK);
    	  break;

      case UWORD_CODE:
    	  number=uint16get();
    	  showWordName(number);
    	  CBK;
    	  break;

      case TH_CODE:
    	  number=uint16get();
    	  consolePrintf("THREAD ");
    	  showWordName(number);
    	  CBK;
    	  break;

      case THP_CODE:
    	  number=uint16get();
    	  consolePrintf("THPRIO ");
    	  showWordName(number);
    	  CBK;
    	  break;

      case TOVAL_CODE:
    	  number=uint16get()-1;
    	  consolePrintf("TO32 ");
    	  showWordName(number);
    	  CBK;
    	  break;

      case TOHVAL_CODE:
    	  number=uint16get()-1;
    	  consolePrintf("TO16 ");
    	  showWordName(number);
    	  CBK;
    	  break;

      case TOCVAL_CODE:
    	  number=uint16get()-1;
    	  consolePrintf("TO8 ");
    	  showWordName(number);
    	  CBK;
    	  break;

      case ADDTOVAL_CODE:
    	  number=uint16get()-1;
    	  consolePrintf("+TO32 ");
    	  showWordName(number);
    	  CBK;
    	  break;

      case ADDTOHVAL_CODE:
    	  number=uint16get()-1;
    	  consolePrintf("+TO16 ");
    	  showWordName(number);
    	  CBK;
    	  break;

      case ADDTOCVAL_CODE:
    	  number=uint16get()-1;
    	  consolePrintf("+TO8 ");
    	  showWordName(number);
    	  CBK;
    	  break;


      default:
    	  showCommand(data);
      }

  }
  while (data!=0);

 CBK; CBK;

 return 0;
 }



// Prints user dictionary word list
// One line for each word giving information about the insides
int32_t userList(ContextType *context,int32_t value)
 {
 UNUSED(value);
 char *name;
 uint16_t pos=0;
 int32_t wsize;

 // Check verbose level
 if (NO_RESPONSE(context)) return 0;

 // Header text
 consolePrintf("%s%sUser dictionary listing:%s%s",BREAK,BREAK,BREAK,BREAK);

 // Start word search
 startWordSearch();

 // Fist word
 name=nextWordSearch(&pos);

 while (name!=NULL)
   {
   // Calculate word size (without header)
   if (previousWord==NO_WORD)
          wsize=UDict.Base.nextPos-pos;
         else
      	  wsize=previousWord-3-UDict.Mem[previousWord-3]-pos;

   consolePrintf("  %s",name);

   switch (UDict.Mem[pos])
     {
     case ENDWORD_CODE:
    	 consolePrintf(" : Null Program");
    	 break;
     case VAR_CODE:
    	 consolePrintf(" = %d (32bit Variable)",*(uint32_t*)(UDict.Mem+pos+1));
    	 if (wsize!=5) { consolePrintf(" Allocates %d bytes!!",wsize-1); }
    	 break;
     case VARH_CODE:
    	 consolePrintf(" = %d (16bit Variable)",*(uint16_t*)(UDict.Mem+pos+1));
    	 if (wsize!=3) { consolePrintf(" Allocates %d bytes!!",wsize-1); }
    	 break;
     case VARC_CODE:
    	 consolePrintf(" = %d (8bit Variable)",*(uint8_t*)(UDict.Mem+pos+1));
    	 if (wsize!=2) { consolePrintf(" Allocates %d bytes!!",wsize-1); }
    	 break;
     case VAL_CODE:
    	 consolePrintf(" = %d (32bit Value)",*(uint32_t*)(UDict.Mem+pos+1));
    	 if (wsize!=5) { consolePrintf(" Allocates %d bytes!!",wsize-1); }
    	 break;
     case VALH_CODE:
    	 consolePrintf(" = %d (16bit Value)",*(uint16_t*)(UDict.Mem+pos+1));
    	 if (wsize!=3) { consolePrintf(" Allocates %d bytes!!",wsize-1); }
    	 break;
     case VALC_CODE:
    	 consolePrintf(" = %d (8bit Value)",*(uint8_t*)(UDict.Mem+pos+1));
    	 if (wsize!=2) { consolePrintf(" Allocates %d bytes!!",wsize-1); }
    	 break;
     case NUM4B_CODE:
    	 if (!UDict.Mem[pos+5])
    	     { consolePrintf(" = %d (32bit Constant)",*(uint32_t*)(UDict.Mem+pos+1)); }
    	   else
    	     { consolePrintf(" : Program word of %d bytes",wsize); }
    	 break;
     case NUM2B_CODE:
    	 if (!UDict.Mem[pos+3])
    	     { consolePrintf(" = %d (16bit Constant)",*(uint16_t*)(UDict.Mem+pos+1)); }
  	        else
  	         { consolePrintf(" : Program word of %d bytes",wsize); }
    	 break;
     case NUM1B_CODE:
    	 if (!UDict.Mem[pos+2])
    	     { consolePrintf(" = %d (8bit Constant)",*(uint8_t*)(UDict.Mem+pos+1)); }
  	        else
  	         { consolePrintf(" : Program word of %d bytes",wsize); }
    	 break;
     case CRT_CODE:
    	 consolePrintf(" : Create word of %d bytes",wsize-1);
    	 break;

     default:
    	 consolePrintf(" : Program word of %d bytes",wsize);
     }

   CBK; // Insert line break

   // Go to next word
   name=nextWordSearch(&pos);
   }

 if (!pos) consolePrintf("  <EMPTY>%s",BREAK);

 consolePrintf("%s",BREAK);

 return 0;
 }

/*************************** LOCAL VARIABLES ******************************/
/*
 Local variables are stored in the return stack
 When a word is executed the return stack pointer is copied
 as the returnStackFrame
 Each local used pops a value from the the parameter stack and pushes it
 on the return stack, so each new local definitions codes >R
 Each following reference to this variable accesses this position on
 the return stack

 Local variables use the definition { vl1 ... vln -- comments... }
 Several local definitions can be used inside a program but not inside
 a do loop as do loops also use the return stack.

 Two hidden words are implemented:
 In both the following byte identifies a position on the return
 stack: ReturnStackFrame+Byte
     GETR Byte   : Gets identified RStack cell and pushes on parameter stack
     SETR Byte   : Pops from stack and sets identified RStack cell

 Compilation:
    For each new local a ">R" is compiled
    At the end of the program the ReturnStackFrame is restored so
    there is no need to drop from return stack
 */


// --- LOCAL PUBLIC FUNCTIONS ------------------------------------

// Code a local in a {    -- } zone
void codeLocalDefinition(char *token)
 {
 // Check that we are not inside a loop
 if (PstackGetSize(&MainContext))
     {
	 consoleErrorMessage(&MainContext,"Cannot define locals inside a loop");
	 return;
     }

 // Check if we are out of local space
 if (nextLocal==MAX_LOCALS)
     {
	 consoleErrorMessage(&MainContext,"No space for more locals");
	 return;
     }

 // Add local to the list
 strCaseCpy(token,LocalNames[nextLocal++]);

 // Code the ">R" word
 if (baseCode(">R"))
     consoleErrorMessage(&MainContext,"Out of memory coding a local");

 //DEBUG_STRING("Added local: ",LocalNames[nextLocal-1]);
 }

// Checks if the token identifies a local variable
// Returns 0 if it is a local
int32_t tokenCheckLocal(char *token)
 {
 int32_t i;

 // Check if there is any local
 if (!nextLocal) return 1; // Not found

 // For all locals
 for(i=0;i<nextLocal;i++)
	 if (!strCaseCmp(token,LocalNames[i]))
	    {
		// Found

		// Check if there is space
		if ((CodePosition+2)>=UD_MEMSIZE)
		    {
			consoleErrorMessage(&MainContext,"Out of memory getting local value");
			return 2;
		    }

		// Code the Get from RStack Hidden Word
		baseCode("GETR");

		//DEBUG_STRING("Code Get Local: ",token);

		// Set the RStack position relative to current fame
		UDict.Mem[CodePosition++]=LocalIndex[i];

		return 0; // Found
	    }

 return 1; // Not found
 }

// --- LOCAL COMMAND FUNCTIONS -----------------------------------

// Implements the local delimiter words
int32_t CodeLocalsDelimiters(ContextType *context,int32_t value)
 {
 int32_t i;

 switch (value)
   {
   case LOCAL_D_START:  // Implements the "{" word
	   // It only can be used out of local mode
	   if (localMode!=NO_LOCAL)
	        {
	        consoleErrorMessage(context,"Improper location of {");
	   	    return 0;
	        }
	   // Set local mode
	   localMode=LOCAL_ENTRY;

	   // Set start of group
	   firstLocalInGroup=nextLocal;

	   //DEBUG_MESSAGE("Locals {");
	   break;

   case LOCAL_D_COMMENT:  // Implements the "--" word
	   // It only can be used in local entry mode
	   if (localMode!=LOCAL_ENTRY)
	        {
	        consoleErrorMessage(context,"Improper location of --");
	   	    return 0;
	        }
	   // Set local mode
	   localMode=LOCAL_COMMENT;

	   //DEBUG_MESSAGE("Locals --");
	   break;

   case LOCAL_D_END:  // Implements the "}" word
	   // It only can be used in a local mode
	   if (localMode==NO_LOCAL)
	        {
	        consoleErrorMessage(context,"Improper location of --");
	   	    return 0;
	        }
	   // Set local mode
	   localMode=NO_LOCAL;

	   //DEBUG_MESSAGE("Locals } Coding indexes");

	   // Check if we have added any local
	   if (firstLocalInGroup==nextLocal) return 0;

	   // Set index for each local
	   for(i=firstLocalInGroup;i<nextLocal;i++)
		   LocalIndex[i]=nextLocal+firstLocalInGroup-i-1;
	   break;
   }

 return 0;
 }

// Executes the SETR hidden word
// Gets following byte and calculates a return stack position:
//    pos=ReturnStackFrame+Byte+1
// Pops a value from parameter stack and puts in RS[pos]
int32_t executeSETR(ContextType *context,int32_t value)
 {
 UNUSED(value);
 int32_t pos,data;

 // Pops one value
 if (PstackPop(context,&data)) return 0;

 // Calculates position
 pos=(int32_t)(context->rstack.Frame)
              +UDict.Mem[(context->Counter)++]+1;

 // Sets Return Stack value
 context->rstack.data[pos]=data;

 return 0;
 }

// Executes the GETR hidden word
// Gets following byte and calculates a return stack position:
//    pos=ReturnStackFrame+Byte+1
// Pushes RS[pos] on the Parameter Stack
int32_t executeGETR(ContextType *context,int32_t value)
 {
 UNUSED(value);
 int32_t pos,data;

 //DEBUG_INT("Return stack frame: ",context->rstack.Frame);
 //DEBUG_INT("Position: ",(int32_t)UDict.Mem[context->Counter]);

 // Calculates position in stack
 pos=(int32_t)(context->rstack.Frame)
              +UDict.Mem[(context->Counter)++]+1;

 // Pushes data on parameter stack
 data=context->rstack.data[pos];
 PstackPush(context,data);

 return 0;
 }

// Executes the ADDR hidden word
// Gets following byte and calculates a return stack position:
//    pos=ReturnStackFrame+Byte+1
// Pops a value from parameter stack and adds it to RS[pos]
int32_t executeADDR(ContextType *context,int32_t value)
 {
 UNUSED(value);
 int32_t pos,data;

 // Pops one value
 if (PstackPop(context,&data)) return 0;

 // Calculates position
 pos=(int32_t)(context->rstack.Frame)
              +UDict.Mem[(context->Counter)++]+1;

 // Sets Return Stack value
 (context->rstack.data[pos])+=data;

 return 0;
 }

// CREATE ------------------------------------------------------

// Executes the CREATE interactive word
int32_t create(ContextType *context,int32_t value)
 {
 UNUSED(value);

 // Start the word definition from a word name from the console
 if (newWordKernel(context)) return 0;

 // Check if there is enough space
 if (getUserMemory()<5)
      {
	  runtimeErrorMessage(context,"Out of memory");
      abortCoding();
      return 0;
      }

 // Code the hidden create
 baseCode("CRT");

 // End the word by making the changes to UDict
 UDict.Base.lastWord=EditWord;
 UDict.Base.nextPos=CodePosition;
 EditWord=NO_WORD;

 return 0;
 }

// Executes the ALLOT interactive word
int32_t allot(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int32_t data;

 // Pop size to allot from the stack
 if (PstackPop(context,&data)) return 0;

 // Check data
 if (data<0)
       {
	   runtimeErrorMessage(context,"Cannot allot negative space");
       return 0;
       }

 // Check if we have enough space
 if (getUserMemory()<data)
       {
	   runtimeErrorMessage(context,"Out of memory");
       return 0;
       }

 // Allot the space
 UDict.Base.nextPos+=data;

 return 0;
 }

// Executes comma interactive word
int32_t comma(ContextType *context,int32_t value)
 {
 int32_t data;
 int32_t *p32;
 int16_t *p16;
 int8_t  *p8;

 // Pop number to set
 if (PstackPop(context,&data)) return 0;

 // Out of memory if less than 4 bytes
 if (getUserMemory()<4)
        {
	    runtimeErrorMessage(context,"Out of memory");
        return 0;
        }

 switch (value)
  {
  case COMMA_32:
	  // Set data
	  p32=(int32_t*)(UDict.Mem+UDict.Base.nextPos);
	  (*p32)=data;
	  // Increase position counter
	  UDict.Base.nextPos+=4;
	  break;
  case COMMA_16:
	  // Set data
	  p16=(int16_t*)(UDict.Mem+UDict.Base.nextPos);
	  (*p16)=(int16_t)data;
	  // Increase position counter
	  UDict.Base.nextPos+=2;
	  break;
  case COMMA_8:
	  // Set data
	  p8=(int8_t*)(UDict.Mem+UDict.Base.nextPos);
	  (*p8)=(int8_t)data;
	  // Increase position counter
	  UDict.Base.nextPos++;
	  break;
  }


 return 0;
 }

// DECOMPILATION -------------------------------------------------------------------

// Decompiles extra bytes in user dictionary as C, values
static void DecompileExtra(int32_t pos,int32_t size)
 {
 int32_t i;
 CBK;
 for(i=0;i<size;i++)
	 consolePrintf("%d C, ",(int8_t)UDict.Mem[pos+i]);
 }

// Calculates relative jump from absolute
static int32_t calculateRelative(int32_t number)
 {
 return (number-decodePosition+3);
 }

// Locates this word
static void locateWord(int32_t pos)
 {
 uint16_t position;

 // Start word search
 startWordSearch();

 do
  {
  nextWordSearch(&position);
  }
  while (position!=pos);

 }

// Decompile a word from its position
int32_t Decompile(int32_t pos)
 {
 int32_t wsize;
 uint8_t data;
 int32_t *p32;
 int16_t *p16;
 int8_t *p8;
 int32_t nword,number;

 // Locate this word
 locateWord(pos);

 // Calculate word size (without header)
 if (previousWord==NO_WORD)
        wsize=UDict.Base.nextPos-pos;
       else
        wsize=previousWord-3-UDict.Mem[previousWord-3]-pos;


 // Print the header
 consolePrintf("\\");
 showWordName(pos);
 consolePrintf(" Decompilation%s",BREAK);

 // Check the kind of word it is
 data=UDict.Mem[pos];

 if (data==VAR_CODE)
     {
	 consolePrintf("VARIABLE ");
	 showWordName(pos);
	 p32=(int32_t*)(UDict.Mem+pos+1);
	 if (*p32)
	    {
	    consolePrintf(" %d ",*p32);
	    showWordName(pos);
	    consolePrintf(" !");
	    }
	 if (wsize>5)
		     DecompileExtra(pos+5,wsize-5);
	 CBK; CBK;
	 return 0;
     }

 if (data==VARH_CODE)
     {
	 consolePrintf("HVARIABLE ");
	 showWordName(pos);
	 p16=(int16_t*)(UDict.Mem+pos+1);
	 if (*p16)
	    {
	    consolePrintf(" %d ",*p16);
	    showWordName(pos);
	    consolePrintf(" H!");
	    }
	 if (wsize>3)
	 		 DecompileExtra(pos+3,wsize-3);
	 CBK; CBK;
	 return 0;
     }

 if (data==VARC_CODE)
     {
	 consolePrintf("CVARIABLE ");
	 showWordName(pos);
	 p8=(int8_t*)(UDict.Mem+pos+1);
	 if (*p8)
	    {
	    consolePrintf(" %d ",*p8);
	    showWordName(pos);
	    consolePrintf(" C!");
	    }
	 if (wsize>2)
	 		 DecompileExtra(pos+2,wsize-2);
	 CBK; CBK;
	 return 0;
     }

 if (data==VAL_CODE)
     {
	 p32=(int32_t*)(UDict.Mem+pos+1);
	 consolePrintf("%d VALUE ",*p32);
	 showWordName(pos);
	 if (wsize>5)
		     DecompileExtra(pos+5,wsize-5);
	 CBK; CBK;
	 return 0;
     }

 if (data==VALH_CODE)
     {
	 p16=(int16_t*)(UDict.Mem+pos+1);
	 consolePrintf("%d HVALUE ",*p16);
	 showWordName(pos);
	 if (wsize>3)
	 		 DecompileExtra(pos+3,wsize-3);
	 CBK; CBK;
	 return 0;
     }

 if (data==VALC_CODE)
     {
	 p8=(int8_t*)(UDict.Mem+pos+1);
	 consolePrintf("%d CVALUE ",*p8);
	 showWordName(pos);
	 if (wsize>2)
	 		 DecompileExtra(pos+2,wsize-2);
	 CBK; CBK;
	 return 0;
     }

 if (data==CRT_CODE)
     {
	 consolePrintf("CREATE ");
	 showWordName(pos);
	 if (wsize>1)
	 	 DecompileExtra(pos+1,wsize-1);
	 CBK; CBK;
	 return 0;
     }

 // Program header
 consolePrintf(": ");
 showWordName(pos);
 CBK;

 // Explore all the word code
 decodePosition=pos;
 do
  {
  // Get data from this position
  data=UDict.Mem[decodePosition++];

  switch (data)
      {
      case EXT1_CODE:
    	  data=UDict.Mem[decodePosition++];
    	  nword=EXT1_START+data;
    	  showCommand(nword);
    	  break;

      case EXT2_CODE:
    	  data=UDict.Mem[decodePosition++];
    	  nword=EXT2_START+data;
    	  showCommand(nword);
    	  break;

      case EXT3_CODE:
    	  data=UDict.Mem[decodePosition++];
    	  nword=EXT2_START+data;
    	  showCommand(nword);
    	  break;

      case NUM1B_CODE:
    	  number=int8get();
    	  consolePrintf("%d%s",number,BREAK);
    	  break;

      case NUM2B_CODE:
    	  number=int16get();
    	  consolePrintf("%d%s",number,BREAK);
    	  break;

      case NUM4B_CODE:
    	  number=int32get();
    	  consolePrintf("%d%s",number,BREAK);
    	  break;

      case SS_CODE:
    	  consolePrintf("S\"");
    	  number=uint8get(); // Get the size
    	  for(;number>0;number--)
    	      		  consolePutChar(UDict.Mem[decodePosition++]);
    	  consolePrintf("\"%s",BREAK);
    	  break;

      case PS_CODE:
    	  consolePrintf(".\"");
    	  while (UDict.Mem[decodePosition]!=0)
    	          consolePutChar(UDict.Mem[decodePosition++]);
    	  decodePosition++;
    	  consolePrintf("\"%s",BREAK);
    	  break;

      case UWORD_CODE:
    	  number=uint16get();
    	  if (number==(uint16_t)pos)
    		  { consolePrintf("RECURSE"); }
    	    else
    	      showWordName(number);
    	  CBK;
    	  break;

      case TH_CODE:
    	  number=uint16get();
    	  consolePrintf("THREAD ");
    	  showWordName(number);
    	  CBK;
    	  break;
      case THP_CODE:
    	  number=uint16get();
    	  consolePrintf("THPRIO ");
    	  showWordName(number);
    	  CBK;
    	  break;
      case TOVAL_CODE:
      case TOHVAL_CODE:
      case TOCVAL_CODE:
    	  number=uint16get()-1;
    	  consolePrintf("TO ");
    	  showWordName(number);
    	  CBK;
    	  break;
      case ADDTOVAL_CODE:
      case ADDTOHVAL_CODE:
      case ADDTOCVAL_CODE:
    	  number=uint16get()-1;
    	  consolePrintf("+TO ");
    	  showWordName(number);
    	  CBK;
    	  break;

      case ENDWORD_CODE:
    	  consolePrintf(";%s",BREAK);
    	  break;

      case JMP_CODE:
    	  number=uint16get();
    	  number=calculateRelative(number);
    	  consolePrintf("[ %d ] JMP%s",number,BREAK);
          break;
      case JZ_CODE:
    	  number=uint16get();
    	  number=calculateRelative(number);
    	  consolePrintf("[ %d ] JZ%s",number,BREAK);
          break;
      case JNZ_CODE:
    	  number=uint16get();
    	  number=calculateRelative(number);
    	  consolePrintf("[ %d ] JNZ%s",number,BREAK);
          break;
      case DO_CODE:
    	  consolePrintf("_DO%s",BREAK);
          break;
      case PLUS_DO_CODE:
    	  number=uint16get();
    	  number=calculateRelative(number);
    	  consolePrintf("[ %d ] P_DO%s",number,BREAK);
          break;
      case MINUS_DO_CODE:
    	  number=uint16get();
    	  number=calculateRelative(number);
    	  consolePrintf("[ %d ] N_DO%s",number,BREAK);
          break;
      case LOOP_CODE:
    	  number=uint16get();
    	  number=calculateRelative(number);
    	  consolePrintf("[ %d ] _LOOP%s",number,BREAK);
          break;
      case ALOOP_CODE:
    	  number=uint16get();
    	  number=calculateRelative(number);
    	  consolePrintf("[ %d ] _@LOOP%s",number,BREAK);
          break;
      case OF_CODE:
    	  number=uint16get();
    	  number=calculateRelative(number);
    	  consolePrintf("[ %d ] _OF%s",number,BREAK);
          break;
      case SETR_CODE:
     	  number=uint8get();
     	  consolePrintf("[ %d ] SETR%s",number,BREAK);
          break;
      case GETR_CODE:
     	  number=uint8get();
     	  consolePrintf("[ %d ] GETR%s",number,BREAK);
          break;
      case ADDR_CODE:
     	  number=uint8get();
     	  consolePrintf("[ %d ] ADDR%s",number,BREAK);
          break;
      default:
    	  showCommand(data);
      }


  }
  while (data!=0);

 CBK; CBK;

 return 0;
 }

/*
		{"SETR","Set RStack value",executeSETR,0,DF_NCOMPILE|DF_BYTE},
		{"GETR","Get RStack value",executeGETR,0,DF_NCOMPILE|DF_BYTE},
		{"ADDR","Add to RStack value",executeADDR,0,DF_NCOMPILE|DF_BYTE},
*/

// Decompiles a word by its name
int32_t DecompileWord(ContextType *context,int32_t value)
 {
 UNUSED(value);

 char *name;
 int32_t pos;

 // Check if info is enabled
 if (NO_RESPONSE(context)) return 0;

 // Get word to dump
 name=tokenGet();

 // Locate this user word
 pos=locateUserWord(name);

 // Error if not found
 if (pos==NO_WORD)
     {
	 consoleErrorMessage(&MainContext,"Word not found");
	 return 0;
     }

 // Decompile the word
 Decompile(pos);

 return 0;
 }


// Decompiles all words
int32_t DecompileAll(ContextType *context,int32_t value)
 {
 UNUSED(value);

 uint16_t last=0,prev;
 uint16_t pos;
 char  *pchar;

 // Check if info is enabled
 if (NO_RESPONSE(context)) return 0;

 // Check if there is any word
 if (UDict.Base.lastWord==NO_WORD)
       {
	   consolePrintf("There are no words in memory%s",BREAK);
	   return 0;
       }

 // Header
 consolePrintf("%sFSTART \\Start of full decompilation%s%s",BREAK,BREAK,BREAK);

 do
  {
  // Start word search
  startWordSearch();

  // Locate next word to display
  pos=NO_WORD;
  do
   {
   prev=pos;
   pchar=nextWordSearch(&pos);
   }
   while ((pos>last)&&(pchar!=NULL));

  last=prev;

  // Check if there is a previous word
  if (prev!=NO_WORD)
	         Decompile(last);
  }
  while(prev!=NO_WORD);

 //Footer
 consolePrintf("FEND \\End of full decompilation%s%s",BREAK,BREAK);

 return 0;
 }

// Compiles the special recompilation words
// JMP JZ JNZ _DO P_DO N_DO _LOOP _@LOOP _OF
int32_t CompileDecompiled(ContextType *context,int32_t value)
 {
 uint16_t *pointer;
 uint8_t *pointer8;
 int32_t data;

 // Check if there is space
 if (getUserMemory()<3)
 	 {
	 consoleErrorMessage(context,"Out of memory");
	 return 0;
 	 }

 // Code the word
 UDict.Mem[CodePosition]=JMP_CODE+value;

 // Return in _DO word because it has no jump address
 if (value==3) { CodePosition++; return 0; }

 // Pop from stack
 if (PstackPop(context,&data)) return 0;

 if (value<=8)
      {
	  // We code a 16 bit relative address
	  // JMP JZ JNZ P_DO N_DO _LOOP _@LOOP _OF

      // Code the address converting to absolute positions
      pointer=(uint16_t*)(UDict.Mem+CodePosition+1);
      (*pointer)=(uint16_t)(CodePosition+data);

      // Increase code position
      CodePosition+=3;
      }
     else
      {
      // We code a 8 bit absolute address
      // SETR GETR ADDR

      // Code the address
      pointer8=(uint8_t*)(UDict.Mem+CodePosition+1);
      (*pointer8)=(uint8_t)(data);

      // Increase code position
      CodePosition+=2;
      }

 return 0;
 }

// STRING FUNCTIONS ---------------------------------------------

int32_t StringFunction(ContextType *context,int32_t value)
 {
 int32_t addr,number;
 uint8_t *p8;

 switch (value)
   {
   case SF_F_COUNT:   // ( addr -- addr+1 u )
	   // Pop the address from the stack
	   if (PstackPop(context,&addr)) return 0;
	   // Convert to pointer to count and increase addr
	   p8=(uint8_t*)addr++;
	   // Push new addr
	   PstackPush(context,addr);
	   // Push count
	   PstackPush(context,(int32_t)*p8);
	   break;

   case SF_F_TYPE:  // ( addr u -- )
	   // Pop count from the stack
	   if (PstackPop(context,&number))return 1;
	   // Pop addr from the stack
	   if (PstackPop(context,&addr)) return 1;
	   // Check count
	   if (number<0)
	     {
		 runtimeErrorMessage(context,"Negative string count");
		 return 0;
	     }
	   // Check verbose
	   if (NO_RESPONSE(context)) return 0;
	   while (number)
	       {
		   consolePutChar((int32_t)*((int8_t*)addr++));
		   number--;
	       }
	   break;

   case SF_F_STYPE: // ( addr -- ) Counted
	   // Pop addr from the stack
	   if (PstackPop(context,&addr)) return 1;
       // Obtain count and increase address
	   number=(int32_t)(*(uint8_t*)addr++);
	   // Check count
	   if (number<0)
	  	   {
	  	   runtimeErrorMessage(context,"Negative string count");
	  	   return 0;
	  	   }
	   // Check verbose
	   if (NO_RESPONSE(context)) return 0;
	   while (number)
	  	   {
	  	   consolePutChar((int32_t)*((int8_t*)addr++));
	  	   number--;
	  	   }
	   break;

   case SF_F_CTYPE: // ( addr -- ) Null terminated
	   // Pop addr from the stack
	   if (PstackPop(context,&addr)) return 1;
	   // Check verbose
	   if (NO_RESPONSE(context)) return 0;
	   // Print it
	   consolePrintf((char *)addr);
	   break;
   }

 return 0;
 }
