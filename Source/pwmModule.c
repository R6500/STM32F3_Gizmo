/*
 pwmModule.c
 PWM functions source file
 */

// Includes
#include "fp_config.h"     // MForth port main config
#include "fp_port.h"       // Foth port include
#include "fm_main.h"       // Forth Main header file
#include "fm_stack.h"      // Stack module header
#include "fm_program.h"
#include "fm_debug.h"
#include "fm_screen.h"
#include "fm_register.h"

#include "gizmo.h"         // Main include for the project
#include "serialModule.h"  // Serial module header
#include "timeModule.h"
#include "pwmModule.h"     // This module header


// PWM Configuration
static PWMConfig pwmcfg;

// Global PWM status
static int32_t pwmStatus=PWMS_OFF;

/****************** STATIC FUNCTIONS *********************/

// Activate PWM module if inactive
static void pwmActivate(void)
 {
 // Do nothing if active
 if (pwmStatus==PWMS_ON) return;

 // Start PWM
 pwmStart(&PWM_DRIVER, &pwmcfg);

 // Set flag
 pwmStatus=PWMS_ON;
 }

// Try to get interval from the stack
// It must be greater than zero
// Returns 0 on error
static int32_t getInterval(ContextType *context,int32_t *num)
 {
 // Try to get from stack
 if (PstackPop(context,num)) return 0;

 // Check that it is positive
 if (((*num)<0)||((*num)>PWM_MAX_INTERVAL))
       {
	   consoleErrorMessage(context,"Invalid PWM interval");
	   return 0;
       }

 return 1; // Ok
 }

// Try to get channel number from the stack
// Returns 0 on error
static int32_t getChannelNumber(ContextType *context,int32_t *num)
 {
 // Try to get from stack
 if (PstackPop(context,num)) return 0;

 // Check that it is valid
 if (((*num)<1)||((*num)>PWM_NLINES))
       {
	   consoleErrorMessage(context,"Invalid PWM channel number");
	   return 0;
       }

 return 1; // Ok
 }

/****************** PUBLIC FUNCTIONS *********************/

// Initializes the PWM module
void pwmModuleInit(void)
 {
 int32_t i;

 // Configure PWM outputs
 palSetPadMode(PWM_PORT,PWM_PIN1,PAL_MODE_ALTERNATE(2));
 palSetPadMode(PWM_PORT,PWM_PIN2,PAL_MODE_ALTERNATE(2));
 palSetPadMode(PWM_PORT,PWM_PIN3,PAL_MODE_ALTERNATE(2));

 // Set default configuration
 pwmcfg.frequency=PWM_DEF_FREQ;
 pwmcfg.period=PWM_DEF_PERIOD;
 pwmcfg.callback=NULL;
 for(i=0;i<3;i++)
    {
	pwmcfg.channels[i].mode=PWM_OUTPUT_ACTIVE_HIGH;
	pwmcfg.channels[i].callback=NULL;
    }
 pwmcfg.channels[3].mode=PWM_OUTPUT_DISABLED;
 pwmcfg.channels[3].callback=NULL;
 }

/****************** COMMAND FUNCTIONS *********************/

// Generic PWM function
int32_t pwmFunction(ContextType *context,int32_t value)
 {
 int32_t nch,number;

 switch (value)
   {
   case PWM_F_CHON: // Activate one channel ( ui uch -- )
	   // Try to get channel number
	   if (!getChannelNumber(context,&nch)) return 0;
	   // Trt to get interval
	   if (!getInterval(context,&number)) return 0;
	   // Activate PWM module if inactive
	   pwmActivate();
	   // Set pwm
	   pwmEnableChannel(&PWM_DRIVER,nch-1,number);
	   break;

   case PWM_F_CHOFF: // Deactivate one channel ( uch -- )
	   // Try to get channel number
	   if (!getChannelNumber(context,&nch)) return 0;
	   // Return if pwm inactive
	   if (pwmStatus==PWMS_OFF) return 0;
	   // Deactivate
	   pwmDisableChannel (&PWM_DRIVER,nch-1);
	   break;

   case PWM_F_FREQ: // Change frequency ( ufreq -- )
	   // Try to get frequency
	   if (!getFreq(context,&number)) return 0;
	   // Stop peripheral if activated
	   if (pwmStatus==PWMS_ON)
	           pwmStop(&PWM_DRIVER);
       // Set frequency
	   pwmcfg.frequency=number;
	   // Restart peripheral if it was active
	   if (pwmStatus==PWMS_ON)
	           pwmStart(&PWM_DRIVER, &pwmcfg);
	   break;

   case PWM_F_PERIOD: // Change period ( ui -- )
	   // Trt to get interval
	   if (!getInterval(context,&number)) return 0;
	   // Store data in driver
	   pwmcfg.period=number;
	   // Change period if activated
	   if (pwmStatus==PWMS_ON)
		       pwmChangePeriod(&PWM_DRIVER,(pwmcnt_t)number);
	   break;

   case PWM_F_STOP: // Stop all the PWM device
	   pwmStop(&PWM_DRIVER);
	   pwmStatus=PWMS_OFF;
	   break;

   }
 return 0;
 }
