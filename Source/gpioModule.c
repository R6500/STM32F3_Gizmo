/*
 gpioModule.c
 GPIO and LED functions
 */

// Includes
#include "fp_config.h"     // MForth port main config
#include "fp_port.h"       // Foth port include
#include "fm_main.h"       // Forth Main header file
#include "fm_stack.h"      // Stack module header

#include "gizmo.h"         // Main include for the project
#include "serialModule.h"  // Serial module header
#include "gpioModule.h"    // This module header


// Variables

// LED Array for the eight leds (Contains bit positions)
const uint16_t LedArray[8]={BIT9,BIT10,BIT11,BIT12,BIT13,BIT14,BIT15,BIT8};

// GPIO Array for the 10 GPIO lines (Contains line numbers)
const uint16_t GpioArray[10]={GPIO0_PIN,GPIO1_PIN,GPIO2_PIN,
		                      GPIO3_PIN,GPIO4_PIN,GPIO5_PIN,
		                      GPIO6_PIN,GPIO7_PIN,GPIO8_PIN,
		                      GPIO9_PIN};

/******************** PUBIC FUNCTIONS ***************************/

// Initializes the GPIO Module
void gpioModuleInit(void)
 {
 int32_t i;

 // All lines in input mode with ODR to zero
 for(i=0;i<10;i++)
    {
	palSetPadMode(GPIO_PORT,GpioArray[i],PAL_MODE_INPUT);
	GpioOff(i);
    }
 }

/******************** COMMAND FUNCTIONS *************************/

// Generic led set/reset function
// Includes:
//     LED_F_SET    Set led n           (a)(n)    -> (a)
//     LED_F_CLEAR  Clear led n         (a)(n)    -> (a)
//     LED_F_WRITE  Give value to led n (a)(v)(n) -> (a)
int32_t ledFunction(ContextType *context,int32_t value)
 {
 int32_t bvalue,data;

 switch (value)
     {
     case LED_F_SET:    // Set one led ------------------
    	 if (!PstackPop(context,&data))   // Pop one value
    	     if ((data>=0)&&(data<=7)) // Check range
    	    	 	 ledOn(data);
    	 break;

     case LED_F_CLEAR:    // Clear one led ------------------
    	 if (!PstackPop(context,&data))   // Pop one value
    	     if ((data>=0)&&(data<=7)) // Check range
    	    	 	 ledOff(data);
    	 break;

     case LED_F_WRITE:    // One led write ------------------
    	 if (PstackGetSize(context)>=2)
      		if (!PstackPop(context,&data))   // Pop led to change
    	       if (!PstackPop(context,&bvalue))   // Pop boolean value
    	           if ((data>=0)&&(data<=7)) // Check range
    	               {
    	    	       if (bvalue)
    	    	 	        ledOn(data);
    	    	           else
    	    	        	ledOff(data);
    	               }
    	 break;

     case LED_F_READ:    // Read  one led status ----------
    	 if (!PstackPop(context,&data))   // Pop one value
    	     if ((data>=0)&&(data<=7)) // Check range
    	         {
    	    	 if (ledRead(data))
    	    		  PstackPush(context,FTRUE);
    	    	     else
    	    	      PstackPush(context,FFALSE);
    	         }
    	 break;

     //default:
     //	 DEBUG_MESSAGE("Cannot arrive to default in ledFunction");
     }

 return 0;
 }

// Generic led binary
// Includes:
//     LED_F_BSET    Set leds binary       (a)(b) -> (a)
//     LED_F_BCLEAR  Clear leds binary     (a)(b) -> (a)
//     LED_F_BVALUE  Leds binary value     (a)(b) -> (a)
int32_t ledBfunction(ContextType *context,int32_t value)
 {
 int32_t data,rvalue,i;

 // Pop one value
 if (PstackPop(context,&data)) return 0;

 // Check range
 if ((data<0)||(data>255)) return 0;

 // Start  with no change
 rvalue=0;

 // Explore all the bits
 for (i=0;i<8;i++)
	 if (data&(1<<i)) // If bit is set
		 rvalue|=LedArray[i];

 switch (value)
     {
     case LED_F_BSET:    // Set leds binary ------------------
    	 LEDS_PORT->BSRR.H.set=rvalue;
    	 break;

     case LED_F_BCLEAR:    // Clear leds binary ------------------
    	 LEDS_PORT->BSRR.H.clear=rvalue;
    	 break;

     case LED_F_BWRITE:    // Leds binary write ------------------
    	 (LEDS_PORT->ODR)=((LEDS_PORT->ODR)&(~LED_MASK))|rvalue;
    	 break;

     //default:
     //	 DEBUG_MESSAGE("Cannot arrive to default in ledBfunction");
     }

 return 0;
 }

// Led binary read
// Reads the status of all the leds
int32_t ledBinaryRead(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int32_t i,result=0;

 // Explore all the bits
 for (i=0;i<8;i++)
	 if ((LEDS_PORT->ODR)&LedArray[i])
		  result|=BIT(i);

 // Push data
 PstackPush(context,result);

 return 0;
 }

// Generic gpio functions
// Includes:
//     GPIO_F_OUTPUT    Set gpio n to output    (a)(n)    -> (a)
//     GPIO_F_OPEN_DRAIN
//     GPIO_F_INPUT
//     GPIO_F_INPUT_UP
//     GPIO_F_INPUT_DOWN
//     GPIO_F_READ
//     GPIO_F_SET
//     GPIO_F_CLEAR
//     GPIO_F_VALUE
int32_t gpioFunction(ContextType *context,int32_t value)
 {
 int32_t line,bvalue;

 switch (value)
     {
     case GPIO_F_OUTPUT:    // Program GPIO to output ------------------
    	 if (!PstackPop(context,&line))    // Pop line to program
    	     if ((line>=0)&&(line<=9)) // Check range
    	    	 	 palSetPadMode(GPIO_PORT,GpioArray[line],PAL_MODE_OUTPUT_PUSHPULL);
       	 break;
     case GPIO_F_OPEN_DRAIN:    // Program GPIO to output in open drain
    	 if (!PstackPop(context,&line))    // Pop line to program
    	     if ((line>=0)&&(line<=9)) // Check range
    	             palSetPadMode(GPIO_PORT,GpioArray[line],PAL_MODE_OUTPUT_OPENDRAIN);
      	 break;

     case GPIO_F_INPUT:    // Program GPIO to input mode ------
    	 if (!PstackPop(context,&line))    // Pop line to program
    	     if ((line>=0)&&(line<=9)) // Check range
    	    	 	 palSetPadMode(GPIO_PORT,GpioArray[line],PAL_MODE_INPUT);
    	 break;

     case GPIO_F_INPUT_UP:    // Program GPIO to input mode with pull-up
    	 if (!PstackPop(context,&line))    // Pop line to program
    	     if ((line>=0)&&(line<=9)) // Check range
    	    	 	 palSetPadMode(GPIO_PORT,GpioArray[line],PAL_MODE_INPUT_PULLUP);
    	 break;

     case GPIO_F_INPUT_DOWN:    // Program GPIO to input mode with pull-down
    	 if (!PstackPop(context,&line))    // Pop line to program
    	     if ((line>=0)&&(line<=9)) // Check range
    	    	 	 palSetPadMode(GPIO_PORT,GpioArray[line],PAL_MODE_INPUT_PULLDOWN);
    	 break;

     case GPIO_F_READ:    // Read GPIO IDR
    	 if (!PstackPop(context,&line))    // Pop line to read
    	     if ((line>=0)&&(line<=9)) // Check range
    	         {
    	    	 if ((GPIO_PORT->IDR)&BIT(GpioArray[line]))
    	    		 PstackPush(context,FTRUE);
    	    	    else
    	    	     PstackPush(context,FFALSE);
    	         }
    	 break;

     case GPIO_F_SET:    // Set GPIO out line
    	 if (!PstackPop(context,&line))    // Pop line to set
    	     if ((line>=0)&&(line<=9)) // Check range
    	    	               GpioOn(line);
       	 break;

     case GPIO_F_CLEAR:    // Clear GPIO out line
    	 if (!PstackPop(context,&line))    // Pop line to set
    	     if ((line>=0)&&(line<=9)) // Check range
    	    	                GpioOff(line);
       	 break;

     case GPIO_F_WRITE:    // One gpio write
    	 if (PstackGetSize(context)>=2)
       		if (!PstackPop(context,&line))   // Pop gpio to change
    	       if (!PstackPop(context,&bvalue))   // Pop boolean value
    	           if ((line>=0)&&(line<=9)) // Check range
    	               {
    	    	       if (bvalue)
    	    	    	    GpioOn(line);
    	    	           else
    	    	        	GpioOff(line);
    	               }

    	 break;

     case GPIO_F_READOUT:    // Read GPIO ODR
    	 if (!PstackPop(context,&line))    // Pop line to read
    	     if ((line>=0)&&(line<=9)) // Check range
    	         {
    	    	 if ((GPIO_PORT->ODR)&BIT(GpioArray[line]))
    	    		 PstackPush(context,FTRUE);
    	    	    else
    	    	     PstackPush(context,FFALSE);
    	         }
    	 break;

     //default:
     //	 DEBUG_MESSAGE("Cannot arrive to default in gpioFunction");
     }

 return 0;
 }

// Gpio binary read FTRUE
// Argument value is not used
int32_t gpioBread(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int32_t data,i;
 uint32_t inputReg;

 // Start  with no data
 data=0;

 // Capture input register
 inputReg=GPIO_PORT->IDR;

 // Explore all the bits
 for (i=0;i<10;i++)
	 if (inputReg&BIT(GpioArray[i])) // If bit is set
		 data|=BIT(i);

 // Push result
 PstackPush(context,data);

 return 0;
 }

// Gpio binary read of output register (ODR)
// Argument value is not used
int32_t gpioBreadOut(ContextType *context,int32_t value)
 {
 UNUSED(value);

 int32_t data,i;
 uint32_t outputReg;

 // Start  with no data
 data=0;

 // Capture input register
 outputReg=GPIO_PORT->ODR;

 // Explore all the bits
 for (i=0;i<10;i++)
	 if (outputReg&BIT(GpioArray[i])) // If bit is set
		 data|=BIT(i);

 // Push result
 PstackPush(context,data);

 return 0;
 }

// Generic gpio binary
// Includes:
//     GPIO_F_BSET    Set gpios binary       (a)(b) -> (a)
//     GPIO_F_BCLEAR
//     GPIO_F_BWRITE
int32_t gpioBfunction(ContextType *context,int32_t value)
 {
 int32_t data,i;
 uint32_t rvalue;

 // Pop one value
 if (PstackPop(context,&data)) return 0;

 // Check range
 if ((data<0)||(data>1023)) return 0;

 // Start  with no change
 rvalue=0;

 // Explore all the bits
 for (i=0;i<10;i++)
	 if (data&(1<<i)) // If bit is set
		 rvalue|=BIT(GpioArray[i]);

 switch (value)
     {
     case GPIO_F_BSET:    // Set gpio binary ------------------
    	 GPIO_PORT->BSRR.H.set=rvalue;
    	 break;

     case GPIO_F_BCLEAR:    // Clear gpio binary ------------------
    	 GPIO_PORT->BSRR.H.clear=rvalue;
    	 break;

     case GPIO_F_BWRITE:    // Leds binary write ------------------
    	 (GPIO_PORT->ODR)=((GPIO_PORT->ODR)&(~GPIO_MASK))|rvalue;
    	 break;

     //default:
     //	 DEBUG_MESSAGE("Cannot arrive to default in gpioBfunction");
     }

 return 0;
 }

// Gpio button read
// Argument is not used
// This function is not currently used as the button
// aborts current running programs.
int32_t gpioButtonRead(ContextType *context,int32_t value)
 {
 UNUSED(value);

 if ((BUTTON_GPIO->IDR)&BIT(BUTTON_PIN))
	 PstackPush(context,1);
    else
     PstackPush(context,0);

 return 0;
 }

