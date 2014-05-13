/***********************************************************************
 *
 *      f m _ f t e s t . c
 *
 * Forth test functions source file
 *
 * Version 1.0 (17/3/2014)
 *
 * Implements functions to test several Forth functionalities
 * during the program development.
 *
 * This file will not be included in the release version
 *
 ***********************************************************************/

// Check if we need to use this file
#ifndef RELEASE_VERSION

// Includes
#include "fp_config.h"     // Main configuration file
#include "fp_port.h"         // Main port definitions
#include "fm_main.h"         // Main forth header file
#include "fm_program.h"      // Program header file
#include "fm_debug.h"        // Debug header file
#include "fm_register.h"     // Register header file
#include "fm_screen.h"       // Screen header file
#include "fm_test.h"         // This module header file

// External definitions

// Local variables in fm_program.c
extern char LocalNames[MAX_LOCALS][MAX_TOKEN_SIZE+1];
extern int8_t LocalIndex[MAX_LOCALS];
extern int16_t nextLocal;

/************************* STATIC FUNCTIONS *****************************/

// Gives information for one Built-inDictionary Word
static void wordTestData(DictionaryEntry *dict,int32_t position)
 {
 char *pointer;

 // Don't check verbose

 CBK;  // Set line break

 // Show word name
 consolePrintf("   %s",dict->name);

 // Check if it is a directive
 if ((dict->flags)&DF_DIRECTIVE)
 		 consolePrintf(" <word>  ");

 // Show flags

 if ((dict->flags)&DF_NCOMPILE)
 		 consolePrintf("   [NO COMPILE]");

 if ((dict->flags)&DF_NI)
		 consolePrintf("   [PROGRAM ONLY]");

 if ((dict->flags)&DF_ADDR)    //TODO This flag will be eliminated
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
			{ consolePrintf(" -> "); }
		   else
	        { consolePutChar(*pointer); }
	    }
	pointer++;
    }

 CBK;  // Set line break

 consolePrintf("   Word position: %d%s",position,BREAK);

 CBK;  // End of help
 }

/************************* COMMAND FUNCTIONS *****************************/

// Show the different data type sizes on the console
int32_t ftestShowTypes(ContextType *context,int32_t value)
 {
 UNUSED(value); UNUSED(context);

 consolePrintString(BREAK);
 consolePrintString("Forth datatype sizes:");
 consolePrintString(BREAK);

 consolePrintString("\tint8_t : ");
 consolePrintInt(sizeof(int8_t));
 consolePrintString(BREAK);

 consolePrintString("\tint16_t : ");
 consolePrintInt(sizeof(int16_t));
 consolePrintString(BREAK);

 consolePrintString("\tint32_t : ");
 consolePrintInt(sizeof(int32_t));
 consolePrintString(BREAK);

 consolePrintString("\tUser Dictionary : ");
 consolePrintInt(sizeof(UserDictionary));
 consolePrintString(BREAK);

 consolePrintString("\tUser Dictionary Base : ");
 consolePrintInt(sizeof(UdictBase));
 consolePrintString(BREAK);

 consolePrintString("\tUser Dictionary Port : ");
 consolePrintInt(sizeof(PortSave));
 consolePrintString(BREAK);

 consolePrintString("\tUser Dictionary Mem size : ");
 consolePrintInt(UD_MEMSIZE);
 consolePrintString(BREAK);

 consolePrintString(BREAK);

 return 0;
 }

// Dump the start of memory
int32_t ftestDumpMem(ContextType *context,int32_t value)
 {
 UNUSED(value); UNUSED(context);

 // Start message
 consolePrintf("%sDump of first 100 UDict positions:%s",BREAK,BREAK);

 // Memory dump
 debugDumpMemory(UDict.Mem,0,100);

 // Show base variables
 consolePrintf("Next position to code : %d%s",UDict.Base.nextPos,BREAK);
 consolePrintf("Last coded word : %d%s",UDict.Base.lastWord,BREAK);

 return 0;
 }

// Show information about built-in words
int32_t ftestBuiltInWords(ContextType *context,int32_t value)
 {
 UNUSED(value); UNUSED(context);

 int32_t pos;
 int32_t cmdSize,hlpSize,maxCmdSize,maxHlpSize;

 // Search in interactive dictionary
 consolePrintf("%sInteractive dictionary:%s",BREAK,BREAK);
 pos=-1;
 maxCmdSize=0;
 maxHlpSize=0;
 while (InteractiveDictionary[++pos].function!=NULL)
    {
	cmdSize=strLen((char*)InteractiveDictionary[pos].name);
	hlpSize=strLen((char*)InteractiveDictionary[pos].help);
	if (maxCmdSize<cmdSize) maxCmdSize=cmdSize;
	if (maxHlpSize<hlpSize) maxHlpSize=hlpSize;
    }
 consolePrintf("  Number of entries: %d%s",pos,BREAK);
 consolePrintf("  Max command length: %d (limit=%d)%s",maxCmdSize,MAX_COMMAND_SIZE,BREAK);
 consolePrintf("  Max help length: %d (limit=%d)%s",maxHlpSize,MAX_HELP_SIZE,BREAK);
 CBK;

 // Search in generator dictionary
 consolePrintf("%sGenerator dictionary:%s",BREAK,BREAK);
 pos=-1;
 maxCmdSize=0;
 maxHlpSize=0;
 while (GeneratorDictionary[++pos].function!=NULL)
    {
	cmdSize=strLen((char*)GeneratorDictionary[pos].name);
	hlpSize=strLen((char*)GeneratorDictionary[pos].help);
	if (maxCmdSize<cmdSize) maxCmdSize=cmdSize;
	if (maxHlpSize<hlpSize) maxHlpSize=hlpSize;
    }
 consolePrintf("  Number of entries: %d%s",pos,BREAK);
 consolePrintf("  Max command length: %d (limit=%d)%s",maxCmdSize,MAX_COMMAND_SIZE,BREAK);
 consolePrintf("  Max help length: %d (limit=%d)%s",maxHlpSize,MAX_HELP_SIZE,BREAK);
 CBK;

 // Search in base dictionary
 consolePrintf("%sBase dictionary:%s",BREAK,BREAK);
 pos=-1;
 maxCmdSize=0;
 maxHlpSize=0;
 while (BaseDictionary[++pos].function!=NULL)
    {
	cmdSize=strLen((char*)BaseDictionary[pos].name);
	hlpSize=strLen((char*)BaseDictionary[pos].help);
	if (maxCmdSize<cmdSize) maxCmdSize=cmdSize;
	if (maxHlpSize<hlpSize) maxHlpSize=hlpSize;
    }
 consolePrintf("  Number of entries: %d%s",pos,BREAK);
 consolePrintf("  Max command length: %d (limit=%d)%s",maxCmdSize,MAX_COMMAND_SIZE,BREAK);
 consolePrintf("  Max help length: %d (limit=%d)%s",maxHlpSize,MAX_HELP_SIZE,BREAK);
 CBK;

 return 0;
 }

// Command that gives some more information than
// the standard "wh" one
int32_t wordTestCommand(ContextType *context,int32_t value)
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
	 wordTestData((DictionaryEntry*)(InteractiveDictionary+pos),pos);
	 found=1;
     }

 // Locate this user word
 upos=locateUserWord(name);
 if (upos!=NO_WORD)
    {
	found=1;
	consolePrintf("Found in User dictionary%s",BREAK);
	consolePrintf("   Position: %u%s%s",upos,BREAK,BREAK);
    }

 // Search in generator dictionary
 pos=searchRegister((DictionaryEntry*) GeneratorDictionary,name);
 if (pos>=0)
     {
	 consolePrintf("Found in Generator dictionary:%s",BREAK);
	 wordTestData((DictionaryEntry*)(GeneratorDictionary+pos),pos);
	 found=1;
     }

 // Search in base dictionary
 pos=searchRegister((DictionaryEntry*) BaseDictionary,name);
 if (pos>=0)
     {
	 consolePrintf("Found in Base dictionary:%s",BREAK);
	 wordTestData((DictionaryEntry*)(BaseDictionary+pos),pos);
	 found=1;
     }

 if (!found)
	 consolePrintf("Word not found%s",BREAK);

 CBK;  // Last line break

 return 0;
 }

// Shows locals from last or current coded program
int32_t localsDump(ContextType *context,int32_t value)
 {
 UNUSED(context); UNUSED(value);
 int32_t i;

 // Check if there is any local
 if (!nextLocal)
    {
	consolePrintf("No local variables defined%s",BREAK);
	return 0;
    }

 for(i=0;i<nextLocal;i++)
    {
	consolePrintf(" %3d : %s%s",LocalIndex[i],LocalNames[i],BREAK);
    }
 CBK;
 return 0;
 }

#endif // RELEASE_VERSION
