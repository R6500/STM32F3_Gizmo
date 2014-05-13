/**************************************************
 *
 *  f m _ s c r e e n . h
 *
 * Screen functions for the Forth project header file
 *
 *************************************************/

#ifndef _FM_SCREEN_MODULE
#define _FM_SCREEN_MODULE

// Possible line breaks
#define BREAK_0 ((char*)BRK_MATRIX+0)   // CR+LF
#define BREAK_1 ((char*)BRK_MATRIX+1)   // CR
#define BREAK_2 ((char*)BRK_MATRIX+2)   // LF

// Line break macro function
#define CBK   consoleBreak()

// Public variables
extern const char BRK_MATRIX[3][3];
extern char *BREAK;

// Console function prototypes
void consoleBreak(void);
void consolePrintString(char *cad);
void consolePrintInt(int32_t value);
void errorEpilogue(void);
void consoleErrorMessage(ContextType *context,char *cad);
void consoleErrorString(ContextType *context,char *cad1, char *cad2);
void consoleErrorInt(ContextType *context,char *cad1, int32_t value);
void consoleWarnMessage(ContextType *context,char *cad);
void runtimeErrorMessage(ContextType *context,char *cad);

// Command function prototypes
int32_t screenFunction(ContextType *context,int32_t value);
#define SF_F_PAGE       0   // Erases and go to to upper left corner
#define SF_F_CR         1   // Print line break
#define SF_F_SPACE      2   // Print a space
#define SF_F_BACKSPACE  3   // Prints a backspace
#define SF_F_CSI        4   // Prints CSI Sequence
#define SF_F_VERBOSE    5   // Set verbose level
#define SF_F_INTPADDED  6   // Implements ".R"
#define SF_F_HEXPADDED  7   // Implements "X.R"

int32_t screenFunctionStack(ContextType *context,int32_t value);
#define SFS_F_DOT       0   // Implements "."
#define SFS_F_DOTHEX    1   // Implements ".X"
#define SFS_F_SPACES    2   // Prints several spaces
#define SFS_F_ATXY      3   // Go to x,y position
#define SFS_F_DOTU      4   // Implements ".U"
#define SFS_F_EMIT      5   // Prints one character
#define SFS_F_SETBREAK  6   // Set line break sequence

int32_t screenColor(ContextType *context,int32_t value);

#endif // _FM_SCREEN_MODULE


