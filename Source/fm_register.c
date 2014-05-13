/*******************************************************************
 *
 *  f m _ r e g i s t e r . c
 *
 * Register functions for the Forth project
 *
 * This module includes the built-in dictionaries
 *
 *        Interactive dictionary - For only interactive words
 *        Generator dictionary   - For compile time words
 *        Base dictionary        - Executable words
 *
 *        In interactive mode we will search in this order:
 *                  - Interactive dictionary
 *                  - Base dictionary
 *                  - User dictionary
 *
 *        In compilation mode we will search in this order:
 *                  - Generator dictionary
 *                  - Base dictionary
 *                  - User dictionary
 *
 *        Any match will stop searching the rest of dictionaries
 *
 ******************************************************************/

// Includes
#include "fp_config.h"      // Main configuration file
#include "fp_port.h"        // Include file for the port
#include "fm_main.h"        // Main header file
#include "fm_stack.h"       // Stack header file
#include "fm_program.h"     // User dictionary header file
#include "fm_debug.h"       // Debug header file
#include "fm_screen.h"      // Screen header file
#include "fm_test.h"        // Test header file
#include "fm_branch.h"      // Branch header file
#include "fm_threads.h"     // Threads header file
#include "fm_register.h"    // This module header file
#include "fp_modules.h"     // Port modules for external Words

// Constant array for the Base Dictionary
// This dictionary includes the words that can be included
// in a compiled program

const DictionaryEntry BaseDictionary[]=
		{
		// Base definitions that cannot be directly compiled

		// End word is included first so that its code will always be zero
		// This word is never executed, but a function "ProgramFunction" is included
	    // otherwise the dictionary search will end
		{"ENDWORD","End of word marker",ProgramFunction,0,DF_NCOMPILE},

		// Extended codes are included her so they code to positions 1, 2 and 3
		{"EXT1","Extended code group 1",ProgramFunction,PF_F_EXTEND1,DF_NCOMPILE},
		{"EXT2","Extended code group 2",ProgramFunction,PF_F_EXTEND2,DF_NCOMPILE},
		{"EXT3","Extended code group 3",ProgramFunction,PF_F_EXTEND3,DF_NCOMPILE},

		// Code for 32,16 and 8 bit signed variables
		// We have three codes but we use the same function
		// They have the codes 4, 5 and 6
		{"VAR","32 bit variable marker",executeVariable,0,DF_NCOMPILE},
		{"VARH","16 bit variable marker",executeVariable,0,DF_NCOMPILE},
		{"VARC","8 bit variable marker",executeVariable,0,DF_NCOMPILE},

        // Number codings
		// They have the codes 7, 8 and 9
		{"1B_NUM","1 byte constant number",int8decode,0,DF_NCOMPILE},
		{"2B_NUM","2 bytes constant number",int16decode,0,DF_NCOMPILE},
		{"4B_NUM","4 bytes constant number",int32decode,0,DF_NCOMPILE},

		// String codings
		// They have the codes 10 and 11
		{"S_STRING","String marker",ProgramFunction,PF_F_S_STRING,DF_NCOMPILE},
		{"P_STRING","Print string marker",ProgramFunction,PF_F_P_STRING,DF_NCOMPILE},

		// User word with code 12
		{"USERWORD","Execute user word",executeUserWord,0,DF_NCOMPILE},

		// Thread from word with codes 13 and 14
        #ifdef USE_THREADS
		{"THRD","Start thread from word marker",threadExecuteFromWord,TEW_NORMAL,DF_NCOMPILE},
		{"THRD_PRIO","Start thread from word marker using priority",threadExecuteFromWord,TEW_PRIORITY,DF_NCOMPILE},
        #else //USE_THREADS
		{"THRD","Start thread from word marker",ProgramFunction,PF_F_WORD_EXCEPTION,DF_NCOMPILE},
		{"THRD_PRIO","Start thread from word marker using priority",ProgramFunction,PF_F_WORD_EXCEPTION,DF_NCOMPILE},
        #endif //USE_THREADS

		// Code for 32,16 and 8 bit signed values
		// They have the codes 15, 16 and 17
		{"VAL","32 bit value marker",executeValue,0,DF_NCOMPILE},
		{"VALH","16 bit value marker",executeHValue,0,DF_NCOMPILE},
		{"VALC","8 bit value marker",executeCValue,0,DF_NCOMPILE},

		// TO Compiled versions
		// They have the codes 18, 19 and 20
		{"TOVAL","Set 32bit value",executeTOVAL,0,DF_NCOMPILE|DF_ADDR},
		{"TOHVAL","Set 16bit value",executeTOHVAL,0,DF_NCOMPILE|DF_ADDR},
		{"TOCVAL","Set 8bit value",executeTOCVAL,0,DF_NCOMPILE|DF_ADDR},

		// ADD TO Compiled versions
		// They have the codes 21, 22 and 23
		{"ADDTOVAL","Add to 32bit value",executeADDTOVAL,0,DF_NCOMPILE|DF_ADDR},
		{"ADDTOHVAL","Add to 16bit value",executeADDTOHVAL,0,DF_NCOMPILE|DF_ADDR},
		{"ADDTOCVAL","Add to 8bit value",executeADDTOCVAL,0,DF_NCOMPILE|DF_ADDR},

		// Create word
		// It uses the code 24
		{"CRT","Create region start",executeVariable,0,DF_NCOMPILE},

		// Branch internal use words
		// Use codes from 25 to 33
		{"JMP","Unconditional jump",Jump,0,DF_NCOMPILE|DF_ADDR},
		{"JZ","Jump if zero",JumpIfZero,0,DF_NCOMPILE|DF_ADDR},
		{"JNZ","Jump if not zero",JumpIfNotZero,0,DF_NCOMPILE|DF_ADDR},
		{"DO","Start of normal loop",ExecuteDO,F_DO_NORMAL,DF_NCOMPILE},
		{"+DO","Start of positive check loop",ExecuteDO,F_DO_PLUS,DF_NCOMPILE|DF_ADDR},
		{"-DO","Start of negative check loop",ExecuteDO,F_DO_MINUS,DF_NCOMPILE|DF_ADDR},
		{"LOOP","End of loop",ExecuteLOOP,0,DF_NCOMPILE|DF_ADDR},
		{"@LOOP","End of loop",ExecuteNewLOOP,0,DF_NCOMPILE|DF_ADDR},
		{"OF","OF in CASE block",ExecuteOF,0,DF_NCOMPILE|DF_ADDR},

		// Words used for local variables
		// They have the codes 34 to 36
		{"SETR","Set RStack value",executeSETR,0,DF_NCOMPILE|DF_BYTE},
		{"GETR","Get RStack value",executeGETR,0,DF_NCOMPILE|DF_BYTE},
		{"ADDR","Add to RStack value",executeADDR,0,DF_NCOMPILE|DF_BYTE},

		// Words from now can be directly compiled ---------------------------

		// Assert check
		// Should not be compiled directrly if it is not from decompiled code
		{"ASRT_CHECK","Assert runtime check",assertCheck,0,DF_NI},

		// Exit and abort commands
		{"EXIT","Exit from current word",ProgramFunction,PF_F_EXIT,DF_NI},
		{"ABORT","Abort to interactive mode",ProgramFunction,PF_F_ABORT,DF_NI},

		// Execute User Dictionary from UDict address
	    {"EXECUTE","Execute from address#(uaddr)$",ProgramFunction,PF_F_EXECUTE_WORD,0},

		// Stack commands implemented in PstackFunction
		{"DROP","Drop stack top#(n)->",PstackFunction,STACK_F_DROP,0},
		{"DROPN","Drop n elements from stack#(a1)..(an)(n)$",PstackFunction,STACK_F_DROP_N,0},
		{"DUP","Duplicate stack top#(n)$(n)(n)",PstackFunction,STACK_F_DUP,0},
		{"?DUP","Duplicate stack top if not zero#0|0->0|(n)$(n)(n)",PstackFunction,STACK_F_DUP_INT,0},
		{"DUPN","Duplicate n elements #(a1)..(an)(n)$(a1)..(an)(a1)..(an)",PstackFunction,STACK_F_DUP_N,0},
		{"PICK","Stack pick element #(ni)..(n0)(i)$(ni)..(n0)(ni)",PstackFunction,STACK_F_PICK,0},
		{"CLEAR","Clears the parameter stack",PstackFunction,STACK_F_CLEAR,0},
		{"SWAPN","Swap top by nth element#(an)..(a0)(n)$(a0)(an-1)..(a1)(an)",PstackFunction,STACK_F_SWAP_N,0},
		{"ROT","Rotates top 3 stack elements#(n3)(n2)(n1)$(n2)(n1)(n3)",PstackFunction,STACK_F_ROT,0},
		{"ROLL","Rotates n+1 stack elememts#(an)..(a0)(n)$(an-1)..(a0)(an)",PstackFunction,STACK_F_ROLL,0},
		{"OVER","Pushest the second element#(n1)(n2)$(n1)(n2)(n1)",PstackFunction,STACK_F_OVER,0},
		{"DEPTH","Showns stack size before this call#$(depth)",PstackFunction,STACK_F_DEPTH,0},
		{"TRUE","Pushes a true value on the stack#$(true)",PstackFunction,STACK_F_TRUE,0},
		{"FALSE","Pushes a false value on the stack#$(false)",PstackFunction,STACK_F_FALSE,0},
		{"UNUSED","Gives user dictionary free memory#$(bytes)",PstackFunction,STACK_F_UNUSED,0},

		// Stack commands implemented in PstackDualFunction
		{"+","Add #(a)(b)$(a+b)",PstackDualFunction,STACK_F_ADD,0},
		{"-","Subtract #(a)(b)$(a-b)",PstackDualFunction,STACK_F_SUB,0},
		{"*","Multiply #(a)(b)$(a*b)",PstackDualFunction,STACK_F_MULT,0},
		{"/","Divide #(a)(b)$(a/b)",PstackDualFunction,STACK_F_DIV,0},
		{"MOD","Modulus #(a)(b)$(a%b)",PstackDualFunction,STACK_F_MOD,0},
		{"SWAP","Stack Swap two top elements#(a)(b)$(b)(a)",PstackDualFunction,STACK_F_SWAP,0},
		{"NIP","Eliminate second stack element#(a)(b)$(a)",PstackDualFunction,STACK_F_NIP,0},
		{"MAX","Find maximum value#(a)(b)$max(a,b)",PstackDualFunction,STACK_F_MAX,0},
		{"MIN","Find minimum value#(a)(b)$min(a,b)",PstackDualFunction,STACK_F_MIN,0},
		{"TUCK","Put top below second#(a)(b)$(b)(a)(b)",PstackDualFunction,STACK_F_TUCK,0},
		{"/MOD","Calculates division and residue#(a)(b)$(a%b)(a/b)",PstackDualFunction,STACK_F_DIV_MOD,0},

		// Relational operators implemented in PstackRelationalFunction
		{"<","Less than#(a)(b)$(a<b)",PstackRelationalFunction,REL_F_LESS,0},
		{">","Greater than#(a)(b)$(a>b)",PstackRelationalFunction,REL_F_GREATER,0},
		{"<=","Less or equal than#(a)(b)$(a<=b)",PstackRelationalFunction,REL_F_L_EQUAL,0},
		{">=","Greater or equal than#(a)(b)$(a>=b)",PstackRelationalFunction,REL_F_G_EQUAL,0},
		{"=","Equal than#(a)(b)$(a=b)",PstackRelationalFunction,REL_F_EQUAL,0},
		{"<>","Different than#(a)(b)$(a!=b)",PstackRelationalFunction,REL_F_UNEQUAL,0},

		// Bitwise operators implemented in PstackBitwiseFunction
		{"INVERT","Bitwise not#(a)$(~a)",PstackBitwiseFunction,BIT_F_NOT,0},
		{"AND","Bitwise And#(a)(b)$(a&b)",PstackBitwiseFunction,BIT_F_AND,0},
		{"OR","Bitwise Or#(a)(b)$(a|b)",PstackBitwiseFunction,BIT_F_OR,0},
		{"XOR","Bitwise Xor#(a)(b)$(a^b)",PstackBitwiseFunction,BIT_F_XOR,0},
		{"LSHIFT","Bitwise Shift Left#(a)(b)$(a<<b)",PstackBitwiseFunction,BIT_F_SHL,0},
		{"RSHIFT","Bitwise Shift Right#(a)(b)$(a>>b)",PstackBitwiseFunction,BIT_F_SHR,0},

		// Unary functions implemented in PstackUnaryFunction
		{"NEGATE","Changes top sign#(a)$(-a)",PstackUnaryFunction,UN_F_NEGATE,0},
		{"NOT","Check against zero#(a)$(a==0)",PstackUnaryFunction,UN_F_NOT,0},
		{"ABS","Absolute value#(a)$(|a|)",PstackUnaryFunction,UN_F_ABS,0},
		{"1+","Increment top#(a)$(a+1)",PstackUnaryFunction,UN_F_INC,0},
		{"1-","Decrement top#(a)$(a-1)",PstackUnaryFunction,UN_F_DEC,0},
		{"2+","Increment top by 2#(a)$(a+2)",PstackUnaryFunction,UN_F_INC2,0},
		{"2-","Decrement top by 2#(a)$(a-2)",PstackUnaryFunction,UN_F_DEC2,0},
		{"2*","Duplicate top#(a)$(a*2)",PstackUnaryFunction,UN_F_DUPLICATE,0},
		{"2/","Halve top#(a)$(a/2)",PstackUnaryFunction,UN_F_HALVE,0},
		{"0<","Check if top < 0#(a)$(a<0)",PstackUnaryFunction,UN_F_0LESS,0},
		{"0>","Check if top > 0#(a)$(a>0)",PstackUnaryFunction,UN_F_0GREAT,0},
		{"0=","Check if top = 0#(a)$(a=0)",PstackUnaryFunction,UN_F_0EQUAL,0},
		{"0<>","Check if top <> 0#(a)$(a<>0)",PstackUnaryFunction,UN_F_0DIFF,0},
		{"CELL+","Add cell size#(a)$(a+Cell_size)",PstackUnaryFunction,UN_F_CELL_PLUS,0},
		{"CELLS","Multiply by cell size#(a)$(a*Cell_size)",PstackUnaryFunction,UN_F_CELLS,0},
		{"HCELL+","Add half cell size#(a)$(a+Cell_size/2)",PstackUnaryFunction,UN_F_HCELL_PLUS,0},
		{"HCELLS","Multiply by half cell size#(a)$(a*Cell_size/2)",PstackUnaryFunction,UN_F_HCELLS,0},

		// Variable functions
		{"@","32 bit Variable Recall#(addr)$(value)",executeVariableRecall,0,0},
		{"!","32 bit Variable Store#(value)(addr)$",executeVariableStore,0,0},
		{"H@","16 bit Variable Recall#(pos)$(value)",executeHVariableRecall,0,0},
		{"H!","16 bit Variable Store#(value)(pos)$",executeHVariableStore,0,0},
		{"C@","8 bit Variable Recall#(pos)$(value)",executeCVariableRecall,0,0},
		{"C!","8 bit Variable Store#(value)(pos)$",executeCVariableStore,0,0},
		{"V@","Intelligent Variable Recall#(pos)$(value)",executeIntelligentVariableRecall,0,0},
		{"V!","Intelligent Variable Store#(value)(pos)$",executeIntelligentVariableStore,0,0},
		{"+!","32 bit Variable Store and add#(+val)(addr)$",executeVariableStoreAdd,0,0},
		{"H+!","16 bit Variable Store and add#(+val)(pos)$",executeHVariableStoreAdd,0,0},
		{"C+!","8 bit Variable Store and add#(+val)(pos)$",executeCVariableStoreAdd,0,0},
		{"V+!","Intelligent Variable Store and add#(+val)(pos)$",executeIntelligentVariableStoreAdd,0,0},

        // Conversion functions
		{"S16U","Convert int16 to uint16#(int16)$(uint16)",PstackUnaryFunction,UN_F_S16U,0},
		{"U16S","Convert uint16 to int16#(uint16)$(int16)",PstackUnaryFunction,UN_F_U16S,0},
		{"S8U","Convert int8 to uint8#(int8)$(uint8)",PstackUnaryFunction,UN_F_S8U,0},
		{"U8S","Convert uint8 to int8#(uint8)$(int8)",PstackUnaryFunction,UN_F_U8S,0},
		{"USER2MEM","Convert user to cpu addresses#(uaddr)$(addr)",PstackUnaryFunction,UN_F_USER2MEM,0},
		{"MEM2USER","Convert cpu to user addresses#(addr)$(uaddr)",PstackUnaryFunction,UN_F_MEM2USER,0},

		// Screen functions
		{"PAGE","Erases screen",screenFunction,SF_F_PAGE,0},
		{"CR","Prints a line break",screenFunction,SF_F_CR,0},
		{"SPACE","Prints a space",screenFunction,SF_F_SPACE,0},
		{"BS","Prints a backspace",screenFunction,SF_F_BACKSPACE,0},
		{"CSI","Prints Control Sequence Introducer",screenFunction,SF_F_CSI,0},
		{"VERBOSE","Sets verbose level#(vLevel)$",screenFunction,SF_F_VERBOSE,0},
		{".R","Prints stack top right justified#(n)(npad)$",screenFunction,SF_F_INTPADDED,0},
		{"X.R","Prints stack top right justified in hexadecimal#(n)(npad)$",screenFunction,SF_F_HEXPADDED,0},

		{".","Prints stack top",screenFunctionStack,SFS_F_DOT,0},
		{"X.","Prints stack top as unsigned hexadecimal",screenFunctionStack,SFS_F_DOTHEX,0},
		{"U.","Prints stack top as unsigned",screenFunctionStack,SFS_F_DOTU,0},
		{"SPACES","Prints n spaces#(n)$",screenFunctionStack,SFS_F_SPACES,0},
		{"AT-XY","Go to screen position (0,0)=ULC#(x)(y)$",screenFunctionStack,SFS_F_ATXY,0},
		{"EMIT","Send one character to screen#(ascii)$",screenFunctionStack,SFS_F_EMIT,0},
		{"SETBREAK","Set line break sequence#(n)$  0:CR+LF 1:CR 2:LF",screenFunctionStack,SFS_F_SETBREAK,0},

		{"COLOR","Set color#(color)$",screenColor,0,0},

		// Words that were only interactive but are moved to normal kind
		// to ease debugging by using script words
	    {".S","Stack dump",PstackList,0,0},
	    // Word lists
	    {"WORDS","Dumps all known words",registerFunction,RF_F_WORDS,0},
	    {"UWORDS","Dumps user words",registerFunction,RF_F_UWORDS,0},
	    {"ULIST","User word list",userList,0,0},
		// Debug commands
	    {"DUMP","Dumps program memory#(start)(length)$",DebugFunction,PF_F_DUMP,0},
	    {"UDATA","Shows user dictionary data information",DebugFunction,PF_F_PGMDATA,0},
	    {"MEMDUMP","Dumps CPU memory#(start)(length)$",DebugFunction,PF_F_MEMDUMP,0},
	    {"SHOWFLAGS","Show global flags",DebugFunction,PF_F_FLAGS,0},
	    {"DB_DEC","Debug with decimal numbers",DebugFunction,PF_F_DEC,0},
	    {"DB_HEX","Debug with hexadecimal numbers",DebugFunction,PF_F_HEX,0},
	    {"LIMITS","Show current MForth limits",DebugFunction,PF_F_LIMITS,0},

		{"UWDUMP","Dumps an user word",UserWordDump,0,DF_DIRECTIVE},
	    {"SEE","See a user word code",See,0,DF_DIRECTIVE},


	    // Return stack commands
	    // Use alias "I" -> "R@"
	    {"RDUMP","Return stack dump",RstackList,0,0},
	    {">R","RS to PS#(n)$ R:$(n)",RstackFunction,RSTACK_F_TO_R,0},
	    {"R>","PS to RS#$(n) R:(n)$",RstackFunction,RSTACK_R_TO_F,0},
	    {"R@","Get RS top without popping#$(n) R:(n)$(n)",RstackFunction,RSTACK_RTOP,0},
	    {"I","Get first do index#$(n) R:(n)$(n)",RstackFunction,RSTACK_GET_I,0},
	    {"J","Get second do index#$(n3) R:(n3)(n2)(n1)$(n3)(n2)(n1)",RstackFunction,RSTACK_GET_J,0},
	    {"K","Get third do index#$(n5) R:(n5)..(n1)$(n5)..(n1)",RstackFunction,RSTACK_GET_K,0},
	    {"RCLEAR","Clear return stack",RstackFunction,RSTACK_CLEAR,0},
	    {"RDROP","Drop top of return stack",RstackFunction,RSTACK_DROP,0},

	    // Branch public words
	    {"UNLOOP","Undo loop effect on return stack#R (n)(n)$",ExecuteUNLOOP,0,0},

	    // Thread words
        #ifdef USE_THREADS
	    {"TLIST","Thread list",threadList,0,0},
	    {"TKILL","Thread kill#(nthread)$",threadKill,0,0},
	    {"TKILLALL","Kill all threads$",threadKillAll,0,0},
        #endif //USE_THREADS

	    // Create words
	    {"CREATE","Create a new data space",create,0,DF_DIRECTIVE},
	    {"ALLOT","Get size bytes of data space#(size)$",allot,0,0},
	    {",","Allocate and set one Cell#(data)$",comma,COMMA_32,0},
	    {"H,","Allocate and set one Half Cell#(data)$",comma,COMMA_16,0},
	    {"C,","Allocate and set one Char#(data)$",comma,COMMA_8,0},

	    // ' <word>
	    {"'","Obtains an user word address#$(Uaddr)",findUserWord,0,DF_DIRECTIVE},

	    // We could need [ ] from inside a word definition
	    {"HERE","Push next code position#$(Uaddr)",ProgramFunction,PF_F_HERE,0},

	    // String functions
	    {"COUNT","Get count from counted string#(addr)$(addr+1)(u)",StringFunction,SF_F_COUNT,0},
	    {"TYPE","Type a counted string from addr and count#(addr)(u)$",StringFunction,SF_F_TYPE,0},
	    {"STYPE","Type a counted string from addr#(addr)$",StringFunction,SF_F_STYPE,0},
	    {"CTYPE","Type a C null terminated string from addr#(addr)$",StringFunction,SF_F_CTYPE,0},

	    {"PAD","Show the PAD address#$(addr)",PstackFunction,STACK_F_PAD,0},


	    // Include all port words
	    #include "fp_portDictionary.h"

	    // Test functions will only be included in non release version ---------------------
        #ifndef RELEASE_VERSION

	    {"TEST_WORDS","Test built-in words",ftestBuiltInWords,0,0},
	    {"TEST_TYPES","Test data types",ftestShowTypes,0,0},
	    {"TEST_DUMP","Dump start of memory",ftestDumpMem,0,0},
	    {"TW","Test word",wordTestCommand,0,DF_DIRECTIVE},
	    {"LOCAL_DUMP","Show local variables",localsDump,0,0},

        #endif //RELEASE_VERSION ----------------------------------------------------------

		// No more functions indicated with NULL pointer
		{"","",NULL,0,0}
		};

// Constant array for the Interactive Dictionary
// This dictionary includes the words that cannot be included
// in a compiled program

const DictionaryEntry InteractiveDictionary[]=
       {
       {":","Start of new word",GenNewWord,0,DF_DIRECTIVE},

       // Duplicate words from interactive Dictionary
       // This duplication is needed for all immediate words
       {"(","Start of comment",immediateFunction,IMM_F_COMMENT,0},
       {".(","Start of echo comment",immediateFunction,IMM_F_DOT_COMMENT,0},

       {"CONSTANT","Create a constant#(value)$",CodeConstant,0,DF_DIRECTIVE},
       {"VARIABLE","Create a 32 bit variable",CodeVariable,0,DF_DIRECTIVE},
       {"HVARIABLE","Create a 16 bit variable",CodeHVariable,0,DF_DIRECTIVE},
       {"CVARIABLE","Create a 8 bit variable",CodeCVariable,0,DF_DIRECTIVE},
       {"VALUE","Create a 32 bit value#(value)$",CodeValue,0,DF_DIRECTIVE},
       {"HVALUE","Create a 16 bit value#(value)$",CodeHValue,0,DF_DIRECTIVE},
       {"CVALUE","Create a 8 bit value#(value)$",CodeCValue,0,DF_DIRECTIVE},
       {"EXECUTE","Execute from address#(uaddr)$",ProgramFunction,PF_F_EXECUTE_INT,0},

       {"TO","Set a value#(value)$",inmediateTO,IT_NORMAL,DF_DIRECTIVE},
       {"+TO","Add to a value#(value)$",inmediateTO,IT_ADD,DF_DIRECTIVE},

       // These are interactive because we cannot erase the running program
       {"FORGET","Forget a user word#Usage: FORGET <word>",UserWordForget,0,0},
       {"FORGETALL","Forget all user words",ProgramFunction,PF_F_FORGETALL,0},

       // Save and load
       {"SAVE","Save the User Dictionary",ProgramFunction,PF_F_SAVE,0},
       {"LOAD","Load the User Dictionary",ProgramFunction,PF_F_LOAD,0},

       // Sets the start word
       {"@START","Set a boot start word",SetStartWord,0,DF_DIRECTIVE},

       {"]","Enter compilation mode",immediateFunction,IMM_F_GO_COMPILE,0},

       // Help
       {"WH","Gives help about a word",wordHelpCommand,0,DF_DIRECTIVE},
       {"BASEWORDS","Gives help about all built-in words",baseWords,0,0},

       // File tag functions
       {"FSTART","Marks the start of a series of lines",ProgramFunction,PF_F_FSTART,0},
       {"FEND","Marks the end of a series of lines",ProgramFunction,PF_F_FEND,0},

       // Debug activation functions
       {"DEBUG-ON","Compile debug and assertions",ProgramFunction,PF_F_DEBUG_ON,0},
       {"DEBUG-OFF","Don't compile debug nor assertions",ProgramFunction,PF_F_DEBUG_OFF,0},

       // Thread words
       #ifdef USE_THREADS
       {"THREAD","Launch a new thread. Returns nthread or 0 on error#$(nthread)",threadLaunch,TL_F_INTERACTIVE,DF_DIRECTIVE},
       {"THPRIO","Launch a new thread with priority#(priority)$(nthread)",threadLaunch,TL_F_INT_PRIO,DF_DIRECTIVE},
       #endif //USE_THREADS

       {"DECOMPILE","Decompile a user word code",DecompileWord,0,DF_DIRECTIVE},
       {"DECOMPILEALL","Decompile the full User Dictionary",DecompileAll,0,0},

       // No more functions indicated with NULL pointer
       {"","",NULL,0,0}
       };

// Constant array for the Generator Dictionary
// This dictionary includes the words are used to generate words
// inside a compiled program

const DictionaryEntry GeneratorDictionary[]=
       {
       {";","End of word",EndNewWord,0,0},

       // Duplicate words from interactive Dictionary
       // This duplication is needed for all immediate words
       {"(","Start of comment",immediateFunction,IMM_F_COMMENT,0},
       {".(","Start of echo comment",immediateFunction,IMM_F_DOT_COMMENT,0},

       {"RECURSE","Call to the word itself",GeneratorFunction,GF_F_RECURSE,0},
       {"[","Enter interactive mode",GeneratorFunction,GF_F_GO_INTERACTIVE,0},
       {"LITERAL","Codes number from stack#(n) ->",GeneratorFunction,GF_F_LITERAL,0},

       // Branch words
       // THEN is an alias of ENDIF
       // AGAIN is an alias of REPEAT
       {"IF","Conditional from IF ELSE ENDIF",CompileIF,0,0},
       {"ELSE","Conditional from IF ELSE ENDIF",CompileELSE,0,0},
       {"ENDIF","Conditional from IF ELSE ENDIF",CompileENDIF,0,0},
       {"THEN","Conditional from IF ELSE THEN",CompileENDIF,0,0},

       {"DO","Start of loop#(limit)(index)$",CompileDO,F_DO_NORMAL,0},
       {"+DO","Start of loop with positive check#(limit)(index)$",CompileDO,F_DO_PLUS,0},
       {"-DO","Start of loop with negative check#(limit)(index)$",CompileDO,F_DO_MINUS,0},
       {"LOOP","End of loop",CompileLOOP,F_LOOP,0},
       {"@LOOP","End of loop with explicit increment#(inc)$",CompileLOOP,F_NEW_LOOP,0},
       {"LEAVE","Exit one loop level",CompileLEAVE,0,0},
       {"?LEAVE","Get top and Exit one loop level if not zero#(flag)$",CompileQ_LEAVE,0,0},

       {"BEGIN","Start of BEGIN UNTIL loop",CompileBEGIN,0,0},
       {"UNTIL","End of BEGIN UNTIL loop#(flag)$",CompileUNTIL,0,0},
       {"WHILE","Part of BEGIN WHILE REPEAT loop#(flag)$",CompileWHILE,0,0},
       {"REPEAT","End of BEGIN WHILE REPEAT loop",CompileREPEAT,0,0},
       {"AGAIN","End of BEGIN AGAIN loop",CompileREPEAT,0,0},

       {"CASE","Start of CASE check#(value)$(value)",CompileCASE,0,0},
       {"OF","CASE comparison#(value)$(value)(tag)",CompileOF,0,0},
       {"ENDOF","End of subblock in CASE#(value)$(value)",CompileENDOF,0,0},
       {"ENDCASE","End of CASE#(value)$",CompileENDCASE,0,0},

       // Assertion words
       {"ASSERT(","Start of assert zone",GeneratorFunction,GF_F_ASRT_START,0},
       {"DEBUG(","Start of debug zone",GeneratorFunction,GF_F_DEBUG_START,0},
       {")","End of assert or debug zone#(code)(flag)$|$",assertEnd,0,0},

       // Thread words
       #ifdef USE_THREADS
       {"THREAD","Launch a new thread. Returns nthread or 0 on error#$(nthread)",threadLaunch,TL_F_COMPILE,DF_DIRECTIVE},
       {"THPRIO","Launch a new thread with priority#(priority)$(nthread)",threadLaunch,TL_F_COM_PRIO,DF_DIRECTIVE},
       #endif //USE_THREADS


       {"TO","Set a value#(value)$",inmediateTO,IT_NORMAL,DF_DIRECTIVE},
       {"+TO","Add to a value#(value)$",inmediateTO,IT_ADD,DF_DIRECTIVE},

       // Local variables
       {"{","Start of local variables definition",CodeLocalsDelimiters,LOCAL_D_START,0},
       {"--","Start of local variables comment",CodeLocalsDelimiters,LOCAL_D_COMMENT,0},
       {"}","Start of local variables definition",CodeLocalsDelimiters,LOCAL_D_END,0},

       // Compilation of decompiled words
       // JMP JZ JNZ _DO P_DO N_DO _LOOP _@LOOP _OF
       {"JMP","Decompiled JMP#(raddr)$",CompileDecompiled,0,0},
       {"JZ","Decompiled JZ#(raddr)$",CompileDecompiled,1,0},
       {"JNZ","Decompiled JNZ#(raddr)$",CompileDecompiled,2,0},
       {"_DO","Decompiled _DO",CompileDecompiled,3,0},
       {"P_DO","Decompiled P_DO#(raddr)$",CompileDecompiled,4,0},
       {"N_DO","Decompiled N_DO#(raddr)$",CompileDecompiled,5,0},
       {"_LOOP","Decompiled _LOOP#(raddr)$",CompileDecompiled,6,0},
       {"_@LOOP","Decompiled _@LOOP#(raddr)$",CompileDecompiled,7,0},
       {"_OF","Decompiled _OF#(raddr)$",CompileDecompiled,8,0},
       {"SETR","Decompiled SETR#(raddr)$",CompileDecompiled,9,0},
       {"GETR","Decompiled GETR#(raddr)$",CompileDecompiled,10,0},
       {"ADDR","Decompiled ADDR#(raddr)$",CompileDecompiled,11,0},

       // No more functions indicated with NULL pointer
       {"","",NULL,0,0}
       };

/******************** STATIC FUNCTIONS *************************/

// Show the words contained in the dictionary
// We show 6 words separated by tabs
static void showWords(DictionaryEntry *pDict)
 {
 int32_t i=0,j,len;

 // While we have not ended the dictionary
 while((pDict->function)!=NULL)
   {
   // Check if it is a non viewable word
   if (!((pDict->flags)&DF_NCOMPILE))
		{
	    if (!i) consolePrintf("  ");
        consolePrintf("%s",pDict->name);

        // Add padding
        len=strLen(pDict->name);

        if (len<18)
          for(j=0;j<18-len;j++)
               { consolePrintf(" "); }

        if ((++i)>=4)
              {
              i=0;
              CBK;
              }
        }
   pDict++;
   }
 }

// Show all built-in dictionaries words
static void showStaticWords(void)
 {
 consolePrintf("%sBase dictionary:%s%s",BREAK,BREAK,BREAK);
 showWords((DictionaryEntry*)BaseDictionary);

 consolePrintf("%s%sInteractive dictionary:%s%s",BREAK,BREAK,BREAK,BREAK);
 showWords((DictionaryEntry*)InteractiveDictionary);

 consolePrintf("%s%sGeneratorDictionary:%s%s",BREAK,BREAK,BREAK,BREAK);
 showWords((DictionaryEntry*)GeneratorDictionary);

 userWordList();

 consolePrintf("%s",BREAK);
 }

/****************** PUBLIC STRING FUNCTIONS ********************/

// Calculates the length of a string
int32_t strLen(char *cad)
 {
 int32_t len=0;

 while ((*cad)!=0) {cad++; len++;};

 return len;
 }

// Copies one string
void strCpy(char *source,char *destination)
 {
 while((*source)!=0)
	 (*(destination++))=(*(source++));
 // Set final terminator
 (*destination)=0;
 }

// Copies one string changing to uppercase
void strCaseCpy(char *source,char *destination)
 {
 while((*source)!=0)
   if (((*source)>='a')&&((*source)<='z'))
	   (*(destination++))=(*(source++))+'A'-'a';
      else
	   (*(destination++))=(*(source++));
 // Set final terminator
 (*destination)=0;
 }

// Compares two strings
// Returns 0 if equal
int32_t strCmp(char *source,char *destination)
 {
 while((*source)!=0)
     {
	 if ((*destination)!=(*source)) return 1;
	 destination++;
	 source++;
     }

 // Check that lenghts match
 if ((*destination)!=0) return 2;

 // Equal contents
 return 0;
 }

// Compares the token with the dictionary entry
// Uses only Uppercase letters in the dictionary
// Returns 0 on match
static int32_t dictUpperCmp(char *dict,char *token)
{
char ch;
while ((*dict)!=0)
   {
   // Jump over lowercase letters
   while (((*dict)<='z')&&((*dict)>='a')) dict++;

   // Check if we arrived to the end
   if (!(*dict)) break;

   // Turn token letter to uppercase
   ch=(*token);
   if ((ch<='z')&&(ch>='a')) ch-=('a'-'A');

   // Check for differences
   if ((*dict)!=ch) return 1;

   // Go to next letter
   dict++;
   token++;
   }

// Check that lenghts match
if ((*token)!=0) return 2;

// Equal contents
return 0;
}

// Compares two strings without considering case
// Returns 0 if equal
int32_t strCaseCmp(char *source,char *destination)
 {
 char c1,c2;

 while((*source)!=0)
     {
	 c1=(*destination);
	 if ((c1<='z')&&(c1>='a')) c1-=('a'-'A');
	 c2=(*source);
	 if ((c2<='z')&&(c2>='a')) c2-=('a'-'A');
	 if (c1!=c2) return 1;
	 destination++;
	 source++;
     }

 // Check that lenghts match
 if ((*destination)!=0) return 2;

 // Equal contents
 return 0;
 }

/****************** PUBLIC DICTIONARY FUNCTIONS ********************/

// Search for a word in one registered dictionary
// Returns the word position in the dictionary
// Returns -1 if not found
int32_t searchRegister(DictionaryEntry *pDict,char *word)
 {
 int32_t i=0;

 // Check if we are at the end
 while ((pDict[i].function)!=NULL)
   {
   // See if we have found it
   if (!strCaseCmp(pDict[i].name,word)) return i;
   // See if it match the alias using uppercase letters
   if (!dictUpperCmp(pDict[i].name,word)) return i;

   // Go to next word
   i++;
   }

 // Not found. Return error
 return -1;
 }

/******************** COMMAND FUNCTIONS **************************************/

// Generic register command function
int32_t registerFunction(ContextType *context,int32_t value)
 {
 UNUSED(context);

 switch (value)
  {
  case RF_F_WORDS:  // Show all words
	  showStaticWords();
      break;

  case RF_F_UWORDS:  // Show user words
	 userWordList();
     consolePrintf("%s",BREAK);
     break;
  }

 return 0;
 }
