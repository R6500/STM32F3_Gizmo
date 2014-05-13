/**************************************************
 *
 *  f m _ r e g i s t e r . h
 *
 * Register functions for the Forth project header file
 *
 *************************************************/

#ifndef _FM_REGISTER_MODULE
#define _FM_REGISTER_MODULE

// Includes


// Function associated to a command (typedef)
typedef int32_t (*cFunction)(ContextType *context,int32_t);

// Dictionary Entry typedef
typedef struct
   {
   char name[MAX_COMMAND_SIZE+1];  // Command name + final null
   char help[MAX_HELP_SIZE+1];     // Help line + final null
   cFunction function;             // Function associated with the command
   int8_t argument;                // Argument in function call
   int8_t flags;                   // Dictionary entry flags
   }DictionaryEntry;

// Possible flags in dictionary entry
#define DF_NI         (BIT(0))     // Command cannot be called in interactive mode

#define DF_ADDR       (BIT(1))     // uint16_t address follows command


#define DF_NCOMPILE   (BIT(2))     // Command cannot be compiled or executed directly
                                   // Like 1, 2 or 4 byte immediate numbers

#define DF_DIRECTIVE  (BIT(3))     // Command directive, a word name follows

#define DF_BYTE       (BIT(4))     // uint8_t follows command

// Fixed register codes (Start of Base Dictionary)
#define ENDWORD_CODE      0
#define EXT1_CODE         1
#define EXT2_CODE         2
#define EXT3_CODE         3
#define VAR_CODE          4
#define VARH_CODE         5
#define VARC_CODE         6
#define NUM1B_CODE        7
#define NUM2B_CODE        8
#define NUM4B_CODE        9
#define SS_CODE          10
#define PS_CODE          11
#define UWORD_CODE       12
#define TH_CODE          13
#define THP_CODE         14
#define VAL_CODE         15
#define VALH_CODE        16
#define VALC_CODE        17
#define TOVAL_CODE       18
#define TOHVAL_CODE      19
#define TOCVAL_CODE      20
#define ADDTOVAL_CODE    21
#define ADDTOHVAL_CODE   22
#define ADDTOCVAL_CODE   23
#define CRT_CODE         24
#define JMP_CODE         25
#define JZ_CODE          26
#define JNZ_CODE         27
#define DO_CODE          28
#define PLUS_DO_CODE     29
#define MINUS_DO_CODE    30
#define LOOP_CODE        31
#define ALOOP_CODE       32
#define OF_CODE          33
#define SETR_CODE        34
#define GETR_CODE        35
#define ADDR_CODE        36

// Public variables
extern const DictionaryEntry BaseDictionary[];
extern const DictionaryEntry InteractiveDictionary[];
extern const DictionaryEntry GeneratorDictionary[];


// Public functions
int32_t strLen(char *cad);
void strCpy(char *source,char *destination);
void strCaseCpy(char *source,char *destination);
int32_t strCmp(char *source,char *destination);
int32_t strCaseCmp(char *source,char *destination);
int32_t searchRegister(DictionaryEntry *pDict,char *word);

// Command functions
int32_t registerFunction(ContextType *context,int32_t value);
#define RF_F_WORDS      1 // Show all words
#define RF_F_UWORDS     2 // Show user words

#endif //_FM_REGISTER_MODULE

