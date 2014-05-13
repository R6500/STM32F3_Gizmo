/**************************************************
 *
 *  f m _ t h r e a d s . h
 *
 *
 * Generic thread operations header file
 *
 * The functions included are port independent
 *
 *************************************************/

#ifdef USE_THREADS

#ifndef _FP_THREADS
#define _FP_THREADS

// As thread details is port dependent we will here only
// the port independent thread information
// Port files should include the port dependent elements

typedef struct
  {
  uint16_t    status;     // Status for this process
  uint16_t    position;   // Execution position
  int16_t     priority;   // Priority respect no normal one
  ContextType context;    // Context for this process
  }
  FThreadData;

// FThread status options
#define FTS_NONE        0   // This thread is not used
#define FTS_RUNNNING    1   // This thread is running

// Function prototypes
void threadsInit(void);
void fThreadStart(void *pointer);
int32_t anythingBackground(void);

// Command functions
int32_t threadList(ContextType *context,int32_t value);
int32_t threadLaunch(ContextType *context,int32_t value);
#define TL_F_INTERACTIVE   0    // Called interactively
#define TL_F_COMPILE       1    // Called from a word definition
#define TL_F_INT_PRIO      2    // Called interactive, gets priority from stack
#define TL_F_COM_PRIO      3    // Called from a word definition, gets priority from stack
int32_t threadExecuteFromWord(ContextType *context,int32_t value);
#define TEW_NORMAL     0    // Normal priority
#define TEW_PRIORITY   1    // Set priority
int32_t threadKill(ContextType *context,int32_t value);
int32_t threadKillAll(ContextType *context,int32_t value);

#endif // _FP_THREADS

#else  // USE_THREADS
#define anythingBackground()      0
#endif // USE_THREADS


