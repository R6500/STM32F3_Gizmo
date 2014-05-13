/*
 timeModule.h
 Time functions header file
 */

#ifndef _TIME_MODULE
#define _TIME_MODULE

// Module definitions
#define N_GPT 2    // Number of generic timers

// Timer defines
#define TIM_DEF_FREQ      10000       //Default frequency
#define TIM_MAX_FREQ      72000000    //f APB1 x 2
#define TIM_MIN_FREQ      1100        //Preescaler limit
#define TIM_MAX_INTERVAL  65535       //16 bit counter

// Timer structure data
typedef struct
 {
 GPTDriver *Driver;   // Driver for this timer
 GPTConfig Config;    // Configureation for this timer
 uint16_t Word;       // Word used as callback
 uint8_t Status;      // Status of the timer
 }
 GenTimer;

// Timer status
#define TSTATUS_STOP	0  // Timer Stoped
#define TSTATUS_RUN     1  // Timer Running

// Function prototypes
void timeInit(void);
int32_t getFreq(ContextType *context,int32_t *freq);
int32_t isAnyTimerCallback(void);

// Command functions
int32_t timeFunction(ContextType *context,int32_t value);
#define  TIME_F_SLEEP        0
#define  TIME_F_TIMER_FREQ   1
#define  TIME_F_TIMER_WORD   2
#define  TIME_F_REPEAT       3
#define  TIME_F_DELAY        4
#define  TIME_F_PAUSE        5
#define  TIME_F_ONE          6
#define  TIME_F_RESET        7
//#define  TIME_F_STOP         8
//#define  TIME_F_INTERVAL     9
//#define  TIME_F_START       10


#endif // _TIME_MODULE


