/*******************************************************************
 *
 *  f m _ b r a n c h . h
 *
 * Branch header file for the Forth project
 *
 * This module implements conditionals and branches
 *
 ******************************************************************/

#ifndef _FM_BRANCH_MODULE
#define _FM_BRANCH_MODULE

// Heads for the parameter stack
// We don't need to use the stack as unsigned
// if we don't use the bit 31

// The heads use the top half of the 32 bit stack values
// We use one bit for each to ease decoding

#define BHEAD_IF       1<<16    // Data is IF FALSE pointer
                                // or ELSE pointer

#define BHEAD_LEAVE    2<<16    // Data is Rstack pointer
                                // at the start of loop

#define BHEAD_DO       3<<16    // Data is DO pointer
                                // to return to start of loop

#define BHEAD_BEGIN    4<<16    // Data is BEGIN address

#define BHEAD_CASE     5<<16    // A case is on the scope
#define BHEAD_OF       6<<16    // There is previous OF

// Heads on the return stack

#define BHEAD_LEAVE    2<<16    // Same as parameter stack
                                // But iddentifies LEAVE addr position


#define BRANCH_MASK      0xFFFF   // Mask to get basic data
#define HEAD_MASK    0x8FFF0000   // Mask to get head data

// Macro functions
#define CPUSH(value)      PstackPush(&MainContext,value)
#define CPOP(pointer)     PstackPop(&MainContext,pointer)
#define RPUSH(value)      RstackPush(&MainContext,value)
#define RPOP(pointer)     RstackPop(&MainContext,pointer)
#define CPUSH_HERE(head)  PstackPush(&MainContext,CodePosition|head)
#define RPUSH_HERE(head)  RstackPush(&MainContext,CodePosition|head)

// Compile public functions
int32_t allocate16u(uint16_t value);

// Compile command functions
int32_t CompileIF(ContextType *context,int32_t value);
int32_t CompileELSE(ContextType *context,int32_t value);
int32_t CompileENDIF(ContextType *context,int32_t value);

int32_t CompileDO(ContextType *context,int32_t value);
#define F_DO_NORMAL 0   // DO
#define F_DO_PLUS   1   // +DO
#define F_DO_MINUS  2   // -DO
int32_t CompileLOOP(ContextType *context,int32_t value);
#define F_LOOP        0
#define F_NEW_LOOP    1
int32_t CompileLEAVE(ContextType *context,int32_t value);
int32_t CompileQ_LEAVE(ContextType *context,int32_t value);

int32_t CompileBEGIN(ContextType *context,int32_t value);
int32_t CompileUNTIL(ContextType *context,int32_t value);
int32_t CompileWHILE(ContextType *context,int32_t value);
int32_t CompileREPEAT(ContextType *context,int32_t value);

int32_t CompileCASE(ContextType *context,int32_t value);
int32_t CompileOF(ContextType *context,int32_t value);
int32_t CompileENDOF(ContextType *context,int32_t value);
int32_t CompileENDCASE(ContextType *context,int32_t value);

// Execute public functions
int32_t getAddrFromHere(ContextType *context);

// Execute command functions
int32_t Jump(ContextType *context,int32_t value);
int32_t JumpIfZero(ContextType *context,int32_t value);
int32_t JumpIfNotZero(ContextType *context,int32_t value);

int32_t ExecuteDO(ContextType *context,int32_t value);
// Same options as CompileDO
//#define F_DO_NORMAL 0   // DO
//#define F_DO_PLUS   1   // +DO
//#define F_DO_MINUS  2   // -DO
int32_t ExecuteLOOP(ContextType *context,int32_t value);
int32_t ExecuteNewLOOP(ContextType *context,int32_t value);
int32_t ExecuteUNLOOP(ContextType *context,int32_t value);
int32_t ExecuteOF(ContextType *context,int32_t value);

#endif // _FM_BRANCH_MODULE

