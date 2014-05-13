/*
 fm_debug.h
 Debug functions header file
 */

#ifndef _FM_DEBUG_MODULE
#define _FM_DEBUG_MODULE

// Conditional compilation macros
#ifdef USE_FDEBUG
#define DEBUG_MESSAGE(a)   debugMessage(a)
#define DEBUG_STRING(a,b)  debugString(a,b)
#define DEBUG_INT(a,b)     debugInt(a,b)
#else
#define DEBUG_MESSAGE(a)   /* NO DEBUG */
#define DEBUG_STRING(a,b)  /* NO DEBUG */
#define DEBUG_INT(a,b)     /* NO DEBUG */
#endif // USE_DEBUG

// Function prototypes
void debugMessage(char *cad);
void debugString(char *cad1, char *cad2);
void debugInt(char *cad1, int32_t i);

void debugDumpMemory(uint8_t *start, uint32_t index, int32_t size);
#define DEBUG_DUMP_LINESIZE    8   // Number of bytes in one debug line

// Command function prototypes
// Command Function Prototypes
int32_t DebugFunction(ContextType *context,int32_t value);
#define PF_F_DUMP         0  // Dump of code
#define PF_F_PGMDATA      1  // Show program data
#define PF_F_MEMDUMP      2  // Memory dump
#define PF_F_FLAGS        3  // Show Static MainFlags
#define PF_F_DEC          4  // Set debug to decimal
#define PF_F_HEX          5  // Set debug to hexadecimal
#define PF_F_LIMITS       6  // Show MForth limits

#endif //_FM_DEBUG_MODULE
