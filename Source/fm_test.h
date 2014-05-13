/***********************************************************************
 *
 *      f m _ t e s t . h
 *
 * Forth test functions header file
 *
 * Version 1.0 (17/3/2014)
 *
 ***********************************************************************/

// Check if we need to use this file
#ifndef RELEASE_VERSION

#ifndef _FM_TEST_MODULE
#define _FM_TEST_MODULE

// Command functions
int32_t ftestShowTypes(ContextType *context,int32_t value);
int32_t ftestDumpMem(ContextType *context,int32_t value);
int32_t ftestBuiltInWords(ContextType *context,int32_t value);
int32_t wordTestCommand(ContextType *context,int32_t value);
int32_t localsDump(ContextType *context,int32_t value);

#endif // _FM_TEST_MODULE

#endif // RELEASE_VERSION


