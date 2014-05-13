/**************************************************
 *
 *  f m _ s t a c k . h
 *
 * Stack functions for the Forth project header file
 *
 *************************************************/

#ifndef _FM_STACK_MODULE
#define _FM_STACK_MODULE

// This define, if enabled, will use vertical stack listings
// in .s and RDUMP commands
// By default, when not selected, standard horizontal list will be shown
//#define VERTICAL_STACK_LIST

// Function macros --------------------------------------

// Pops one value from the stack as unsigned number
// Pointer should be a pointer to uint32_t
#define PstackPopUnsigned(context,pointer) PstackPop(context,(int32_t*)pointer)

// Parameter stack function prototypes
void PstackInit(ContextType *context);
void PstackPrintTop(ContextType *context);
void PstackPush(ContextType *context,int32_t value);
int32_t PstackPop(ContextType *context,int32_t *value);
int32_t PstackGetSize(ContextType *context);
int32_t PstackGetTop(ContextType *context,int32_t *value);
void PstackClone(ContextType *cBase,ContextType *cCopy);

// Parameter stack command functions -------------------

int32_t PstackFunction(ContextType *context,int32_t value);
#define STACK_F_DROP      1
#define STACK_F_DUP       2
#define STACK_F_PICK      3
#define STACK_F_CLEAR     4
#define STACK_F_DROP_N    5
#define STACK_F_DUP_N     6
#define STACK_F_SWAP_N    7
#define STACK_F_ROT       8
#define STACK_F_ROLL      9
#define STACK_F_DUP_INT  10
#define STACK_F_OVER     11
#define STACK_F_DEPTH    12
#define STACK_F_TRUE     13
#define STACK_F_FALSE    14
#define STACK_F_UNUSED   15
#define STACK_F_PAD      16

int32_t PstackDualFunction(ContextType *context,int32_t value);
#define STACK_F_ADD       1
#define STACK_F_SUB       2
#define STACK_F_MULT      3
#define STACK_F_DIV       4
#define STACK_F_MOD       5
#define STACK_F_SWAP      6
#define STACK_F_NIP       7
#define STACK_F_MAX       8
#define STACK_F_MIN       9
#define STACK_F_TUCK     10
#define STACK_F_DIV_MOD  11

int32_t PstackRelationalFunction(ContextType *context,int32_t value);
#define REL_F_LESS        1
#define REL_F_GREATER     2
#define REL_F_L_EQUAL     3
#define REL_F_G_EQUAL     4
#define REL_F_EQUAL       5
#define REL_F_UNEQUAL     6

int32_t PstackBitwiseFunction(ContextType *context,int32_t value);
#define BIT_F_NOT         1
#define BIT_F_AND         2
#define BIT_F_OR          3
#define BIT_F_XOR         4
#define BIT_F_SHL         5
#define BIT_F_SHR         6

int32_t PstackUnaryFunction(ContextType *context,int32_t value);
#define UN_F_NEGATE       1
#define UN_F_NOT          2
#define UN_F_ABS          3
#define UN_F_INC          4
#define UN_F_DEC          5
#define UN_F_INC2         6
#define UN_F_DEC2         7
#define UN_F_DUPLICATE    8
#define UN_F_HALVE        9
#define UN_F_0LESS       11
#define UN_F_0GREAT      12
#define UN_F_0EQUAL      13
#define UN_F_0DIFF       14
#define UN_F_CELL_PLUS   15
#define UN_F_CELLS       16
#define UN_F_S16U        17
#define UN_F_U16S        18
#define UN_F_S8U         19
#define UN_F_U8S         20
#define UN_F_USER2MEM    21
#define UN_F_MEM2USER    22
#define UN_F_HCELL_PLUS  23
#define UN_F_HCELLS      24

int32_t PstackList(ContextType *context,int32_t value);

// Return stack function prototypes ----------------------
void RstackInit(ContextType *context);
int32_t RstackPush(ContextType *context,int32_t value);
int32_t RstackPop(ContextType *context,int32_t *value);
int32_t RstackGetTop(ContextType *context,int32_t *value);
int32_t RstackGetSize(ContextType *context);
int32_t RstackAddTop(ContextType *context,int32_t value);

// Parameter stack command functions
int32_t RstackList(ContextType *context,int32_t value);

int32_t RstackFunction(ContextType *context,int32_t value);
#define RSTACK_F_TO_R  0
#define RSTACK_R_TO_F  1
#define RSTACK_RTOP    2
#define RSTACK_GET_I   3
#define RSTACK_GET_J   4
#define RSTACK_CLEAR   5
#define RSTACK_DROP    6
#define RSTACK_GET_K   7

#endif //_FM_STACK_MODULE

