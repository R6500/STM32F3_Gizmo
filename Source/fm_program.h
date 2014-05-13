/*******************************************************************
 *
 *  f m _ p r o g r a m . h
 *
 * Program functions for the Forth project header file
 *
 * This module implements the user dictionary
 *
 ******************************************************************/

#ifndef _FM_PROGRAM_MODULE
#define _FM_PROGRAM_MODULE

// User dictionary definitions ---------------------------------------

typedef struct
 {
 uint32_t magic;        // Magic number
 uint16_t lastWord;     // Last word position in dictionary
 uint16_t nextPos;      // Next position to use in dictionary
 uint16_t startWord;    // Position of boot start word (or NO_WORD)
 } UdictBase;

#define RAW_UDICT_MEM_SIZE     (USER_DICT_SIZE-sizeof(UdictBase)-sizeof(PortSave))
#define UD_MEMSIZE             (((int)(RAW_UDICT_MEM_SIZE/4)-1)*4)

typedef struct
 {
 UdictBase Base;
 PortSave  Port;
 uint8_t   Mem[UD_MEMSIZE];
 } UserDictionary;

// Magic to detect if there is a Program Memory in flash
// Version number is used to change Magic on different versions
#define MAGIC_NUMBER   (0xF03234+FVERSION_INT)

// Definitions for binary codes in memory
#define MAX_NORMAL_CODE  249   // Max Code number of base commands (240 normal commands)
#define CODE_EXTEND1     250   // Indentificator for first extended code
#define CODE_EXIT        255   // Code for exit of binary mode (Not implemented yet)

 // Limits for integer codification
 #define MIN_1B_INT   -128
 #define MAX_1B_INT    127
 #define MIN_2B_INT   -32768
 #define MAX_2B_INT    32767
 #define MIN_4B_INT   ((int32_t)(1<<31))
 #define MAX_4B_INT   ((int32_t)(~(1<<31)))

// Local variables modes
#define NO_LOCAL       0   // No "{" word yet
#define LOCAL_ENTRY    1   // Word "{" introduced
#define LOCAL_COMMENT  2   // Word "--" introduced

// Public variables
extern UserDictionary  UDict; // User dictionary
extern uint32_t EditWord;     // Currently edited word

// Function Prototypes
void programErase(void);
void programInit(void);
void userWordList(void);
uint16_t locateUserWord(char *name);
int32_t  getUserMemory(void);
void codePrintString(char *pointer);
void codeString(char *pointer);
void showWordName(int32_t addr);

// User Coding Functions
void abortCompile(void);
int32_t baseCode(char *word);
void programCodeNumber(int32_t value);
int32_t codeUserPosition(uint16_t position);

// Datatype coding
int32_t CodeConstant(ContextType *context,int32_t value);
int32_t CodeVariable(ContextType *context,int32_t value);
int32_t CodeHVariable(ContextType *context,int32_t value);
int32_t CodeCVariable(ContextType *context,int32_t value);
int32_t CodeValue(ContextType *context,int32_t value);
int32_t CodeHValue(ContextType *context,int32_t value);
int32_t CodeCValue(ContextType *context,int32_t value);

int32_t create(ContextType *context,int32_t value);
int32_t allot(ContextType *context,int32_t value);
int32_t comma(ContextType *context,int32_t value);
#define COMMA_32    0 // Codes ","
#define COMMA_16    1 // Codes "H,"
#define COMMA_8     2 // Codes "C,"

// Local variable coding
int32_t CodeLocalsDelimiters(ContextType *context,int32_t value);
#define LOCAL_D_START   0  // Codes "{"
#define LOCAL_D_COMMENT 1  // Codes "--"
#define LOCAL_D_END     2  // Codes "}"
void codeLocalDefinition(char *token);
int32_t tokenCheckLocal(char *token);
// Local variable execution
int32_t executeSETR(ContextType *context,int32_t value);
int32_t executeGETR(ContextType *context,int32_t value);
int32_t executeADDR(ContextType *context,int32_t value);

// User execute and interactive functions
int32_t SetStartWord(ContextType *context,int32_t value);
void programExecute(ContextType *context,uint16_t position,int32_t primary);
int32_t int8decode(ContextType *context,int32_t value);
int32_t int16decode(ContextType *context,int32_t value);
int32_t int32decode(ContextType *context,int32_t value);
int32_t executeUserWord(ContextType *context,int32_t value);
int32_t executeVariable(ContextType *context,int32_t value);
int32_t executeVariableRecall(ContextType *context,int32_t value);
int32_t executeVariableStore(ContextType *context,int32_t value);
int32_t executeHVariableRecall(ContextType *context,int32_t value);
int32_t executeHVariableStore(ContextType *context,int32_t value);
int32_t executeCVariableRecall(ContextType *context,int32_t value);
int32_t executeCVariableStore(ContextType *context,int32_t value);
int32_t executeIntelligentVariableRecall(ContextType *context,int32_t value);
int32_t executeIntelligentVariableStore(ContextType *context,int32_t value);
int32_t executeVariableStoreAdd(ContextType *context,int32_t value);
int32_t executeHVariableStoreAdd(ContextType *context,int32_t value);
int32_t executeCVariableStoreAdd(ContextType *context,int32_t value);
int32_t executeIntelligentVariableStoreAdd(ContextType *context,int32_t value);
int32_t executeValue(ContextType *context,int32_t value);
int32_t executeHValue(ContextType *context,int32_t value);
int32_t executeCValue(ContextType *context,int32_t value);

int32_t wordHelpCommand(ContextType *context,int32_t value);
int32_t baseWords(ContextType *context,int32_t value);

// TO word is used both in compile and interactive mode
int32_t inmediateTO(ContextType *context,int32_t value);
#define IT_NORMAL    0    // Just stores to value
#define IT_ADD       1    // Adds to value
// TO compiled executables
int32_t executeTOVAL(ContextType *context,int32_t value);
int32_t executeTOHVAL(ContextType *context,int32_t value);
int32_t executeTOCVAL(ContextType *context,int32_t value);
// ADD TO compiled executables
int32_t executeADDTOVAL(ContextType *context,int32_t value);
int32_t executeADDTOHVAL(ContextType *context,int32_t value);
int32_t executeADDTOCVAL(ContextType *context,int32_t value);

// Command Function Prototypes
// TODO Set incremental numbers from 0
int32_t ProgramFunction(ContextType *context,int32_t value);
#define PF_F_S_STRING         3  // Constant string
#define PF_F_P_STRING         4  // Print string
#define PF_F_EXTEND1          7  // Extended code 250..499
#define PF_F_EXTEND2          8  // Extended code 500..749
#define PF_F_EXTEND3          9  // Extended code 750..999
#define PF_F_EXIT            10
#define PF_F_ABORT           11
#define PF_F_EXECUTE_INT     12
#define PF_F_EXECUTE_WORD    13
#define PF_F_FORGETALL       14
#define PF_F_SAVE            15
#define PF_F_LOAD            16
#define PF_F_HERE            17
#define PF_F_FSTART          18
#define PF_F_FEND            19
#define PF_F_DEBUG_ON        20
#define PF_F_DEBUG_OFF       21
#define PF_F_WORD_EXCEPTION  22

// Extended code zones
#define EXT1_START      250
#define EXT1_END        499
#define EXT2_START      500
#define EXT2_END        749
#define EXT3_START      750
#define EXT3_END        999


int32_t GeneratorFunction(ContextType *context,int32_t value);
#define GF_F_RECURSE          0  // Recursive call
#define GF_F_GO_INTERACTIVE   1  // Implement "["
#define GF_F_LITERAL          2  // Pops stack and codes it
#define GF_F_ASRT_START       3  // Start of assert zone
#define GF_F_DEBUG_START      4  // Start of debug zone

int32_t findUserWord(ContextType *context,int32_t value);
int32_t UserWordDump(ContextType *context,int32_t value);
int32_t UserWordForget(ContextType *context,int32_t value);

int32_t GenNewWord(ContextType *context,int32_t value);
int32_t EndNewWord(ContextType *context,int32_t value);
int32_t EndNewWordWithoutEnd(void);

int32_t assertEnd(ContextType *context,int32_t value);
int32_t assertCheck(ContextType *context,int32_t value);

int32_t See(ContextType *context,int32_t value);
int32_t DecompileWord(ContextType *context,int32_t value);
int32_t CompileDecompiled(ContextType *context,int32_t value);
int32_t DecompileAll(ContextType *context,int32_t value);

int32_t userList(ContextType *context,int32_t value);

int32_t StringFunction(ContextType *context,int32_t value);
#define SF_F_COUNT     0
#define SF_F_TYPE      1
#define SF_F_STYPE     2
#define SF_F_CTYPE     3

#endif // _FM_PROGRAM_MODULE


