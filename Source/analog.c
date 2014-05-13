/*
 analog.c
 Analog ADC functions
 */

// Includes
#include "fp_config.h"     // MForth port main config
#include "fp_port.h"       // MFoth port include
#include "fm_main.h"       // MForth Main header file
#include "fm_stack.h"      // Stack module header
#include "fm_debug.h"      // Debug module
#include "fm_screen.h"
#include "fm_program.h"    // Program module


#include "gizmo.h"         // Main include for the project
#include "chprintf.h"	   // chprintf function
#include "analog.h"        // This module header file

// Channel definitions
const AnalogChannel AChannels[NUM_CHANNELS]={
		         {ANALOG0_PORT,ANALOG0_PIN,ANALOG0_CHANNEL},
		         {ANALOG1_PORT,ANALOG1_PIN,ANALOG1_CHANNEL},
		         {ANALOG2_PORT,ANALOG2_PIN,ANALOG2_CHANNEL},
		         {ANALOG3_PORT,ANALOG3_PIN,ANALOG3_CHANNEL},
		         {ANALOG4_PORT,ANALOG4_PIN,ANALOG4_CHANNEL},
		         {ANALOG5_PORT,ANALOG5_PIN,ANALOG5_CHANNEL},
		         };

// Number of samples to make a mean value
int32_t nmean=ANALOG_NMEAN;

// Value of Vdd in mV
int32_t VddCal=VDD_NOT_CALIBRATED;

// Extern variables
extern UserDictionary  UDict;   // User dictionary in fm_program.h

/****************** PRIVATE FUNCTIONS ********************/

// Enable ADC1 and ADC2
static void analogADCenable(void)
 {
 ADC1->CR|=ADC_CR_ADEN;
 ADC2->CR|=ADC_CR_ADEN;
 while (!((ADC1->ISR)&(ADC_ISR_ADRDY)));  // Wait to start ADC1
 while (!((ADC2->ISR)&(ADC_ISR_ADRDY)));  // Wait to start ADC2
 }

// Disable ADC1 and ADC2
static void analogADCdisable(void)
 {
 ADC1->CR|=ADC_CR_ADDIS;
 ADC2->CR|=ADC_CR_ADDIS;
 while (!((ADC1->CR)&(ADC_CR_ADEN)));  // Wait to stop ADC1
 while (!((ADC2->CR)&(ADC_CR_ADEN)));  // Wait to stop ADC2
 }

// Set channel to differential
// Every channel can be set to differential mode
// the next channel (n+1) will be the reference
// and should not be used in single mode
static void analogSetDifferential(int32_t channel)
 {
 analogADCdisable();
 ADC1->DIFSEL|=BIT(channel);
 ADC2->DIFSEL|=BIT(channel);
 analogADCenable();
 }

// Set channel to single mode
static void analogSetSingle(int32_t channel)
 {
 analogADCdisable();
 ADC1->DIFSEL&=(~BIT(channel));
 ADC2->DIFSEL&=(~BIT(channel));
 analogADCenable();
 }


/****************** PUBLIC FUNCTIONS *********************/

// Calibrates ADC1,2 for single channel
// ADC must be stopped to do that
// Calibration for single and differential mode are independent
// If both modes are used both calibrations need to be done
void analogCalibrateSingle(void)
 {
 // Calibrate ADC1 in single channel mode
 ADC1->CR|=BIT(31);
 while  ((ADC1->CR)&BIT(31)); // Wait to end calibration

 // Calibrate ADC2 in single channel mode
 ADC2->CR|=BIT(31);
 while  ((ADC2->CR)&BIT(31)); // Wait to end calibration
 }

// Calibrates ADC1,2 for differential mode
// ADC must be stopped to do that
// Calibration for single and differential mode are independent
// If both modes are used both calibrations need to be done
void analogCalibrateDifferential(void)
 {
 // Set ADC1 to calibrate in differential mode
 ADC1->CR|=BIT(30);

 // Calibrate ADC1
 ADC1->CR|=BIT(31);
 while  ((ADC1->CR)&BIT(31)); // Wait to end calibration

 // Set ADC2 to calibrate in differential mode
 ADC2->CR|=BIT(30);

 // Calibrate ADC2
 ADC2->CR|=BIT(31);
 while  ((ADC2->CR)&BIT(31)); // Wait to end calibration
 }

// Inits the analog subsystem
// It also calibrates the
void analogInit(void)
 {
 int i;

 // Activates the ADC1,2 clock
 RCC->AHBENR|=RCC_AHBENR_ADC12EN;

 // We set the ADC1,2 clock frequency
 // It can operate at the full 72MHz
 // We divided by 4 the AHB clock (72MHz)
 // so ADC1,2 clocks are 18 MHz
 ADC1_2->CCR|=3<<16;

 // ADC Sample Time
 // For slot 1
 // 110: 181.5 ADC clock cycles
 // That corresponds to 10us
 ADC1->SMPR1|=6<<3;
 ADC2->SMPR1|=6<<3;

 // Activate the internal Vref and Temperature sensor
 // Sampling time must be greater that 2.2us
 ADC1_2->CCR|=BIT(22)|BIT(23);

 // Calibrate the ADC for single mode
 analogCalibrateSingle();

 // Calibrate the ADC for differential mode
 analogCalibrateDifferential();

 // Activates ADC1,2
 analogADCenable();

 // Mux of the analog pads
 for (i=0;i<NUM_CHANNELS;i++)
      palSetPadMode(AChannels[i].port,AChannels[i].pin,PAL_MODE_INPUT_ANALOG);

 // DAC Initialization -----------------------------------------------
 // We use DAC1_OUT1

 // We enable the APB1 DAC clock
 RCC->APB1ENR |= RCC_APB1ENR_DACEN;

 // We eanble channel 1
 DAC->CR|=DAC_CR_EN1;

 // We set the output to zero by default
 DAC->DHR12R1=0;

 // Mux DAC channel
 palSetPadMode(DAC_PORT,DAC_PIN,PAL_MODE_INPUT_ANALOG);

 // Calibrate Vdd ---------------

 // Wait 0.5s
 SLEEP_MS(500);

 // Calibrate
 analogCalibrateVdd();
 }

// Uses ADC1 to make a conversion
// ADC clock is 18 MHz
// Sample time is 181,5 cycles
// ADC conversion is 12 cycles
// Total time is 10,75 us
int32_t adc1Convert(int32_t channel)
 {
 int32_t value;

 // We indicate the channel
 ADC1->SQR1=(channel&0x1F)<<6;

 // Start of conversion
 ADC1->CR|=BIT(2);

 // Wait for conversion to end
 while (!((ADC1->ISR)&(ADC_ISR_EOC)));

 // Read the conversion
 value=ADC1->DR;

 return value;
 }

// Uses ADC1 to make "mean" conversions
// And return the mean value
// Total time for default 100 cycles is about 1ms
int32_t adc1ConvertMean(int32_t channel)
 {
 int i, result=0;

 // Discard first reading
 adc1Convert(channel);

 // Add all readings
 for(i=0;i<nmean;i++)
	 result+=adc1Convert(channel);

 // Return the mean
 result=result/nmean;

 return result;
 }

// Uses ADC1 to read the internal reference
// Its value should be 1.16 < 1.2V < 1.25V
// The typical value should be
// around 1638 for Vdd=3V
int32_t adc1Reference(void)
 {
 int32_t i,result=0;

 // We indicate the Vref channel
 ADC1->SQR1=18<<6;

 // Do 110 conversions
 for (i=0;i<110;i++)
     {
     // Start of conversion
     ADC1->CR|=BIT(2);

     // Wait for conversion to end
     while (!((ADC1->ISR)&(ADC_ISR_EOC)));

     // Accumulate data on last 100 conversions
     if (i>=10) result+=ADC1->DR;
     }

 // Calculate mean
 result=result/100;

 return result;
 }

// Reads the internal Vref and uses it to obtain the Vdd value
// This value is stored in the VddCal variable
void analogCalibrateVdd(void)
 {
 int32_t nref;

 // Obtain vref conversion counts
 nref=adc1Reference();

 // Calculate Vdd value
 VddCal=UDict.Port.vref4096/nref;
 }

// Calculates the real internal reference from value
// given from a real mesurement of Vdd
// Updates Program Memory from the results
int32_t analogMeasuredVdd2Vref(int32_t VddMeas)
 {
 int32_t Vref,V4096;

 // Set VddCal to this value
 VddCal=VddMeas;

 // Calculate reference in mv and in 4096 product
 Vref=(VddMeas*adc1Reference())/4096;
 V4096=(VddMeas*adc1Reference())/1000;

 // Set real reference to program memory
 UDict.Port.vref4096=V4096;

 // Gives the result
 return Vref;
 }

// Uses ADC1 to read the internal temperature
// The result is given in 10*celsius
// Sampling time must be at least 2.2us
int32_t adc1Temperature(void)
 {
 int32_t i,result=0;

 // We indicate the Temp channel
 ADC1->SQR1=16<<6;

 // Do 110 conversions
 for (i=0;i<110;i++)
     {
     // Start of conversion
     ADC1->CR|=BIT(2);

     // Wait for conversion to end
     while (!((ADC1->ISR)&(ADC_ISR_EOC)));

     // Accumulate data on last 100 conversions
     if (i>=10) result+=ADC1->DR;
     }

 // Calculate mean
 result=result/100;

 // Show mean value for debug
 DEBUG_INT("Raw temperature counts: ",result);

 // Calculate temperature
 result=V25_40960-30*result;
 result=result/SLOPE_4096;
 result=result+250;

 return result;
 }

// Uses ADC1 and ADC2 to make two conversions at the same time
// Returns the values using pointers
//
// In theory the ADC must be disabled to change from single
// to dual operation, but the implemented method works ok
void adc1ConvertDual(int32_t channel1,int32_t channel2,int32_t *value1,int32_t *value2)
 {
 // Disable ADCs prior to programming
 //analogADCdisable();

 // We program ADC2 for simultaneous operation
 ADC1_2->CCR|=6;

 // Enable ADCs after programming
 //analogADCenable();

 // We indicate the channel in ADC1
 ADC1->SQR1=(channel1&0x1F)<<6;

 // We indicate the channel in ADC2
 ADC2->SQR1=(channel2&0x1F)<<6;

 // Start of conversion
 ADC1->CR|=BIT(2);

 // Wait for conversion to end
 //while (!((ADC1_2->CSR)&BIT(2)));
 while (!((ADC1->ISR)&(ADC_ISR_EOC)));

 // Disable ADCs prior to programming
 //analogADCdisable();

 // We program ADC2 for independent operation
 ADC1_2->CCR&=(~31);

 // Enable ADCs after programming
 //analogADCenable();

 // Read the conversion results
 (*value1)=ADC1->DR;
 (*value2)=ADC2->DR;
 }

// Uses ADC1,2 to make "mean" simultaneous conversions
// Total time for default 100 cycles is about 1ms
void adc1ConvertDualMean(int32_t channel1,int32_t channel2,int32_t *value1,int32_t *value2)
 {
 int i;
 int32_t result1,result2;

 // Start zeroing values
 (*value1)=0;
 (*value2)=0;

 // Add all readings
 for(i=0;i<nmean;i++)
     {
	 adc1ConvertDual(channel1,channel2,&result1,&result2);
	 (*value1)=(*value1)+result1;
	 (*value2)=(*value2)+result2;
     }

 // Return the mean
 (*value1)=(*value1)/nmean;
 (*value2)=(*value2)/nmean;
 }

/******************** COMMAND FUNCTIONS *************************/

// Generic analog function
// Includes:
//     ANALOG_F_READ    Read channel n           (a)(n)    -> (a)(value)
//
//
int32_t analogFunction(ContextType *context,int32_t value)
 {
 int32_t channel,channel2,data,data2;

 switch (value)
     {
     case ANALOG_F_READ:    // Read one channel ------------------
    	 if (!PstackPop(context,&channel))   // Pop one value
    	     {
    	     if ((channel>=0)&&(channel<NUM_CHANNELS)) // Check range
    	             {
    	    	     data=adc1Convert(AChannels[channel].channel);
    	    	     PstackPush(context,data);
    	             }
    	            else
    	             runtimeErrorMessage(context,"Invalid analog channel");
    	     }
         break;

     case ANALOG_F_READ_MEAN:    // Read one channel with mean ------------------
    	 if (!PstackPop(context,&channel))   // Pop one value
    	     {
    	     if ((channel>=0)&&(channel<NUM_CHANNELS)) // Check range
    	             {
    	    	     data=adc1ConvertMean(AChannels[channel].channel);
    	    	     PstackPush(context,data);
    	             }
    	            else
    	             runtimeErrorMessage(context,"Invalid analog channel");
    	     }
         break;

     case ANALOG_F_READ_VREF:    // Read reference
    	 data=adc1Reference();
    	 PstackPush(context,data);
         break;

     case ANALOG_F_DAC:    // Set DAC value
    	 if (!PstackPop(context,&data))   // Pop one value
    	     {
    		 if ((data>=0)&&(data<4096))  // Check range
    			 DAC->DHR12R1=data;
    		    else
    		     runtimeErrorMessage(context,"Invalid DAC value");
    	     }
         break;

     case ANALOG_F_2READ:    // 2 simultaneous reads
    	 if (PstackGetSize(context)<2)
    	         {
    		     runtimeErrorMessage(context,"Not enough elements");
    	         }
    	      	else
    	      	 {
    	         PstackPop(context,&channel2);  // Get channel for ADC2
    	         PstackPop(context,&channel);   // Get channel for ADC1
    	         if (!channel2)
    	                  {
    	        	      runtimeErrorMessage(context,"A0 cannot be used in ADC2");
    	        	      return 0;
    	                  }
    	         adc1ConvertDual(AChannels[channel].channel
    	        		         ,AChannels[channel2].channel
    	        		         ,&data,&data2);  // Convert
    	         PstackPush(context,data);      // Push ADC1 conversion
    	         PstackPush(context,data2);     // Push ADC2 conversion
    	      	 }
         break;

     case ANALOG_F_2READ_MEAN:    // 2 simultaneous reads with mean
    	 if (PstackGetSize(context)<2)
    	         {
    		     runtimeErrorMessage(context,"Not enough elements");
    	         }
    	      	else
    	      	 {
    	         PstackPop(context,&channel2);  // Get channel for ADC2
    	         PstackPop(context,&channel);   // Get channel for ADC1
    	      	 if (!channel2)
    	      	          {
    	      	   	      runtimeErrorMessage(context,"A0 cannot be used in ADC2");
    	      	   	      return 0;
    	      	          }
    	         adc1ConvertDualMean(AChannels[channel].channel
    	        		             ,AChannels[channel2].channel
    	        		             ,&data,&data2);  // Convert
    	         PstackPush(context,data);      // Push ADC1 conversion
    	         PstackPush(context,data2);     // Push ADC2 conversion
    	      	 }
          break;

     case ANALOG_F_SINGLE:    // Set Channel to single 0..4
    	 if (!PstackPop(context,&channel))   // Pop one value
    	     {
    		 if ((channel>=0)&&(channel<5))  // Check range
    			 analogSetSingle(AChannels[channel].channel);
    		   else
    			 runtimeErrorMessage(context,"Invalid analog channel");
    	     }
         break;

     case ANALOG_F_DIFFERENTIAL:    // Set Channel to differential 0..4
    	 if (!PstackPop(context,&channel))   // Pop one value
    	     {
    		 if ((channel>=0)&&(channel<5))  // Check range
    			 analogSetDifferential(AChannels[channel].channel);
    		   else
    			 runtimeErrorMessage(context,"Invalid analog channel");
    	     }
         break;

     case ANALOG_F_NMEAN:    // Set number of samples for mean
    	 if (!PstackPop(context,&data))   // Pop one value
    		 if ((data>0)&&(data<=500000))  // Check range
    			 nmean=data;   // Set number of samples to mean
         break;

     case ANALOG_F_READ_T:    // Read Temperature
    	 data=adc1Temperature();
    	 PstackPush(context,data);
         break;

     case ANALOG_F_CAL:    // Calibrate Vdd
    	 analogCalibrateVdd();
    	 PstackPush(context,VddCal);  // Push calibration value (in mV)
         break;

     case ANALOG_F_SINGLE_CONVERT:    // Convert to mV for single measurement
    	 if (!PstackPop(context,&data))   // Pop one value
    	     {
    		 // Convert to mV
    		 if (VddCal==VDD_NOT_CALIBRATED)
    		      data=(data*3000)/4096;
    		     else
    			  data=(data*VddCal)/4096;

    		 // Push data
    		 PstackPush(context,data);
    	     }
          break;

     case ANALOG_F_DIFFERENTIAL_CONVERT:   // Convert to mV for differential measurement
    	 if (!PstackPop(context,&data))   // Pop one value
    	     {

    		 // Change to signed value
    		 data=data-2048;

    		 // Convert to mV
    		 if (VddCal==VDD_NOT_CALIBRATED)
    		      data=(data*3000)/2048;
    		     else
    			  data=(data*VddCal)/2048;

    		 // Push data
    		 PstackPush(context,data);
    	     }
          break;

     case ANALOG_F_USE_VDDMEAS:    // Use measured Vdd. Gives internal reference
    	 if (!PstackPop(context,&data))   // Pop one value
    	     {
    		 // Call to function and obtain internal reference
    		 data2=analogMeasuredVdd2Vref(data);

    		 // Push internal reference (in mV)
    		 PstackPush(context,data2);
    	     }
          break;

     case ANALOG_F_MV2COUNTS:    // Convert from mv to counts
    	 if (!PstackPop(context,&data))   // Pop one value
    	     {
    		 // Convert to counts
    		 if (VddCal==VDD_NOT_CALIBRATED)
    		       data=(data*4096)/3000;
    		     else
    		       data=(data*4096)/VddCal;

    		 // Verify limits
    		 if (data>4095) data=4095;

    		 // Push converted count value
    		 PstackPush(context,data);
    	     }
          break;

     case ANALOG_F_USE_VREF:    // Use known Vref. Gives Vdd value
    	 if (!PstackPop(context,&data))   // Pop one value
    	     {
    		 // Store the data in Program Memory
    		 UDict.Port.vref4096=data*4096;

    		 // Recalibrate from known reference
    		 analogCalibrateVdd();

    		 // Push Vdd value (in mV)
    		 PstackPush(context,VddCal);
    	     }
          break;

     default:
	    DEBUG_MESSAGE("Cannot arrive to default in analogFunction");
     }

 return 0;
 }

/*********************************** TEST FUNCTIONS ******************************************/

#ifdef TEST_ANALOG //------------------------------------------------------ TEST_ANALOG STARTS

// Read analog channel #0 with ADC1 (PF4 ch#5)
static void test1channel(void)
{
int i,value;

analogInitSingle();
consolePrintf("%sAnalog module initializaed%s",BREAK,BREAK);

consolePrintf("%d reads on channel 0 (PF4 ADC1 ch#5)%s",ANALOG_TEST_NREADS,BREAK);
for(i=0;i<ANALOG_TEST_NREADS;i++)
    {
	 value=adc1Convert(ANALOG0_CHANNEL);
	 consolePrintf("%d%s",value,BREAK);  // Without caption
	 //chprintf(Console_BSS,"\tAnalog value: %d%s",value,BREAK); // With caption
    }

}

// Read two channels in sequence
//    analog channel #0 with ADC1 (PF4 ch#5)
//    analog channel #1 with ADC1 (PC0 ch#6)
static void test2channelSequence(void)
{
int i,value1,value2;

analogInit();
consolePrintf("%sAnalog module initializaed%s",BREAK,BREAK);

consolePrintf("%d reads on channel 0 and 1%s",ANALOG_TEST_NREADS,BREAK);
consolePrintf("CH0 (PF4)\tCH1 (PC0)%s",BREAK);
for(i=0;i<ANALOG_TEST_NREADS;i++)
    {
	 value1=adc1Convert(ANALOG0_CHANNEL);
	 value2=adc1Convert(ANALOG1_CHANNEL);
	 consolePrintf("%d\t%d%s",value1,value2,BREAK);  // Without caption
	 //consolePrintf("\tAnalog value: %d%s",value,BREAK); // With caption
    }

}

// Read analog channels #0 and #1 with ADC1 in differential mode (PF4 ch#5 - PC0 ch#6)
static void testDifferential(void)
{
int i,value;

analogInitDifferential();
consolePrintf(Console_BSS,"%sAnalog module initializaed%s",BREAK,BREAK);

chprintf("%d reads on channels 0,1 in differential mode%s",ANALOG_TEST_NREADS,BREAK);
for(i=0;i<ANALOG_TEST_NREADS;i++)
    {
	 value=adc1Convert(ANALOG0_CHANNEL);
	 consolePrintf("%d%s",value,BREAK);  // Without caption
	 //consolePrintf("\tAnalog value: %d%s",value,BREAK); // With caption
    }

}

// Analog test entry point
void analogTest(void)
 {
 //test1channel();
 //test2channelSequence();
 testDifferential();
 }

#endif // -------------------------------------------------------------- TEST_ANALOG ENDS

