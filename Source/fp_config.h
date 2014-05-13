/**************************************************
 *
 * f p _ c o n f i g . h
 *
 * STM32F3 Gizmo configuration file
 *
 * PORT FILE : This file is implementation dependent
 *
 * This file provides general definitions and compilation
 * options for the MForth system.
 *
 *************************************************/

#ifndef _FP_CONFIG
#define _FP_CONFIG

// MForth version information -----------------------------------------

// Version in text mode
#define FVERSION       "1.0"

// Version in integer mode (100*version)
#define FVERSION_INT    100

// Release definition
// Activate for final release version that disables test words
#define RELEASE_VERSION

// Size and limits definitions ----------------------------------------

// Max number of simultaneous threads
#define MAX_THREADS    4

// Number of flash pages to use for the program memory
#define FLASH_PAGES     14

// Max length of a console line (in chars)
#define MAX_CONSOLE_LINE 256

// Max length of one word token (don't affect strings)
#define MAX_TOKEN_SIZE    32

// Size of each context Parameter Stack
#define STACK_SIZE        50

// Size of each context Return Stack
#define RSTK_SIZE         50

// Max number of local values in a word
#define MAX_LOCALS        10

// Size and limits definitions specific for the STM32F3Gizmo port ------

// Max number of user semaphores 0..
#define MAX_SEMAPHORES	10

// Max number of user mutexes 0..
#define MAX_MUTEXES     10

// Register size limits ------------------------------------------------

// These limits affect the static dictionary arrays

// Use TEST_WORDS to adjust the following defines
// to minimize the memory footprint after release

// Max length of a command word name
#define MAX_COMMAND_SIZE  24

// Max length of the help line for a command
#define MAX_HELP_SIZE    100

// Conditional compilation of features --------------------------------

// If enabled, debug info will be send to console
//#define USE_FDEBUG

// If enabled, thread words will be compiled
#define USE_THREADS

// Post processing calculations ---------------------------------------

// User dictionary size is calculated from flash pages
// It has to be multiple of four
#define USER_DICT_SIZE    (FLASH_PAGES*2048)

#endif // _FP_CONFIG
