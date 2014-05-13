/***********************************************************************
 *
 *      f m _ m a i n . h
 *
 * Main forth functions header file
 *
 * Version 1.0 (17/3/2014)
 *
 ***********************************************************************/

#ifndef _FM_MAIN_MODULE
#define _FM_MAIN_MODULE

// Forth version information
#define FVERSION_DATE  "20/4/2014"   // Manual date version is now automatic
//#define FVERSION_DATE  __DATE__      // Automatic date version is now automatic

// Bit definition
#define BIT(n) (1<<(n))

// Verbose level bits
#define VBIT_ECHO       (BIT(0))   //   1 :Echo from console
#define VBIT_ERROR      (BIT(1))   //   2 :Error messages
#define VBIT_RESPONSE   (BIT(2))   //   4 :Asked responses as cp, cs ...
#define VBIT_TOP        (BIT(3))   //   8 :Show stack top after command if modified
#define VBIT_INFO       (BIT(4))   //  16 :Help and general info
#define VBIT_PROMPT     (BIT(5))   //  32 :Command prompt
#define VBIT_DEBUG      (BIT(6))   //  64 :Debug messages
#define VERBOSE_ALL     (VBIT_ECHO|VBIT_ERROR|VBIT_RESPONSE|VBIT_TOP \
                         |VBIT_INFO|VBIT_PROMPT|VBIT_DEBUG)

// Verbose Mask in file mode
#define FILE_VERBOSE_MASK    (VBIT_ECHO|VBIT_ERROR|VBIT_RESPONSE \
                             |VBIT_INFO|VBIT_DEBUG)
// Verbose selected in file error mode
#define FILE_ERROR_VERBOSE    0

// Macro to check if context indicates foreground
#define IS_FOREGROUND(context)     ((context->Process)==FOREGROUND)
#define NOT_FOREGROUND(context)    (context->Process)

// Macros associated to verbose level
#define NO_ECHO                  (!((MainContext.VerboseLevel)&VBIT_ECHO))
#define NO_ERROR(context)        (!((context->VerboseLevel)&VBIT_ERROR))
#define SHOW_ERROR(context)      (((context)->VerboseLevel)&VBIT_ERROR)
#define NO_RESPONSE(context)     (!( (context->VerboseLevel)&VBIT_RESPONSE ))
#define SHOW_RESPONSE(context)   ( (context->VerboseLevel)&VBIT_RESPONSE )
#define NO_TOP                   (!((MainContext.VerboseLevel)&TOP))
#define SHOW_INFO(context)       ((context->VerboseLevel)&VBIT_INFO)
#define NO_INFO(context)         (!((context->VerboseLevel)&VBIT_INFO))
#define NO_PROMPT                (!((MainContext.VerboseLevel)&VBIT_PROMPT))
#define NO_DEBUG                 (!((MainContext.VerboseLevel)&VBIT_DEBUG))

// No edited word
#define NO_WORD   0xFFFF

// Definition of true and false
#define FTRUE   -1
#define FFALSE   0

// Flags of the MainFlags variable
#define MFLAG_LOADED      (BIT(0))   // Data was loaded
#define MFLAG_START_WORD  (BIT(1))   // Start word was executed
#define MFLAG_IERROR      (BIT(2))   // Interpretation error flag
#define MFLAG_CERROR      (BIT(3))   // Compile error flag
#define MFLAG_FERROR      (BIT(4))   // File error flag
#define MFLAG_FILE        (BIT(5))   // Input file mode
#define MFLAG_DEBUG_ON    (BIT(6))   // Debug activated
#define MFLAG_ASSERT_ZONE (BIT(7))   // We are inside an assert zone
                                     // Between "assert(" and ")"
#define MFLAG_DB_HEX      (BIT(8))   // Debug in hexadecimal
#define MFLAG_DEBUG_ZONE  (BIT(9))   // We are inside an debug zone
                                     // Between "debug(" and ")"

// Unused macro to eliminate warning
#define UNUSED(x) (void)(x)

// Stack typedefs --------------------------------------------------------

typedef struct  // Circular parameter stack typedef
     {
	 int16_t Pointer;            // Pointer to current position
	 int16_t Size;               // Current number of elements
	 int32_t data[STACK_SIZE];   // Stack data
     } StackType;

typedef struct // Return Stack typedef
    {
	int16_t Frame;            // Frame at the start of a word
	int16_t Pointer;          // Pointer to current position
	int32_t data[RSTK_SIZE];  // Stack data
    } RStackType;

// The return stack frame is the value of the pointer upon
// entering the execution of a word

// Typedef for context data -------------------------------------------------
// Define the environment context where a program runs
typedef struct
    {
  	StackType stack;                    // Stack for this context
  	RStackType rstack;                  // Return stack for this context

    uint16_t Counter;                   // Run program counter
   	int16_t  Process;                   // Process 0=Foreground
   	uint32_t Flags;                     // Context Flags
   	uint32_t VerboseLevel;              // Context Verbose Level
  } ContextType;

// Context Flags values
#define CFLAG_EXIT    (BIT(0))       // Exit current word execution
#define CFLAG_ABORT   (BIT(1))       // Abort all context execution

// Flags that end current word
#define CFLAGS_ENDWORD  (CFLAG_EXIT|CFLAG_ABORT)

#define FOREGROUND 0

// Public variables
//extern  uint32_t VerboseLevel;
extern ContextType MainContext;
extern ContextType InterruptContext;
extern uint32_t MainFlags;

// Macro functions
#define AbortExecution(context)    (context->Flags)|=CFLAG_ABORT
#define ExitWord(context)          (context->Flags)|=CFLAG_EXIT
#define IN_FOREGROUND(context)     ((context->Process)==FOREGROUND)

// Function prototypes
void clearInterruptContext(void);
int32_t checkTokenLength(char *token);
char *tokenGet(void);
int32_t Fatoi(char *cad,int32_t *value);
char *Fitoa(int32_t num, char *str, int32_t radix);
void forthInit(void);
void forthMain(void);
void abortLine(void);
void showCapabilities(void);

// Command functions
int32_t immediateFunction(ContextType *context,int32_t value);
#define IMM_F_COMMENT        1  //  ( comment
#define IMM_F_DOT_COMMENT    2  // .( comment
#define IMM_F_GO_COMPILE     3  // "]"
#define IMM_F_RL             4  // Repeat last line

#endif //_FM_MAIN_MODULE

