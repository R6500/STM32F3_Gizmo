/*
 analog.h
 Analog module header file

 Functions to operate with the ADCs
 */

#ifndef _ANALOG_MODULE
#define _ANALOG_MODULE

// Defines

// Number of reads on analog test
#define ANALOG_TEST_NREADS   128

// Typedef that holds data about a channel
typedef struct
    {
	GPIO_TypeDef *port;
	int32_t pin;
	int32_t channel;
    }AnalogChannel;

// Arguments of analogFunction
#define ANALOG_F_READ                    1
#define ANALOG_F_READ_MEAN               2
#define ANALOG_F_READ_VREF               3
#define ANALOG_F_DAC                     4
#define ANALOG_F_2READ                   5
#define ANALOG_F_2READ_MEAN              6
#define ANALOG_F_SINGLE                  7
#define ANALOG_F_DIFFERENTIAL            8
#define ANALOG_F_NMEAN                   9
#define ANALOG_F_READ_T                 10
#define ANALOG_F_CAL                    11
#define ANALOG_F_SINGLE_CONVERT         12
#define ANALOG_F_DIFFERENTIAL_CONVERT   13
#define ANALOG_F_USE_VDDMEAS            14
#define ANALOG_F_MV2COUNTS              15
#define ANALOG_F_USE_VREF               16

// Value of VddCal variable when not calibrated
#define VDD_NOT_CALIBRATED    -1

// Typical vref data
#define Vref_4096     4915200    // Vref(mv) * 4096 corresponds to 1.2V
                                 // From datasheet can be 1.16 < 1.2 < 125

// Temperature sensor data for 3V Vdd
#define V25_40960     58573   // V25*40960
#define SLOPE_4096    17      // SLOPE*4096

// Function prototypes
void analogCalibrateSingle(void);
void analogCalibrateDifferential(void);
void analogInit(void);
int32_t adc1Convert(int32_t channel);
int32_t adc1ConvertMean(int32_t channel);
int32_t adc1Reference(void);
void analogCalibrateVdd(void);
int32_t analogMeasuredVdd2Vref(int32_t VddMeas);
int32_t adc1Temperature(void);
void adc1ConvertDual(int32_t channel1,int32_t channel2,int32_t *value1,int32_t *value2);
void adc1ConvertDualMean(int32_t channel1,int32_t channel2,int32_t *value1,int32_t *value2);

// Command functions
int32_t analogFunction(ContextType *context,int32_t value);

#ifdef TEST_ANALOG //------------------------------------------------------ TEST_ADC STARTS
void analogTest(void);
#endif // -------------------------------------------------------------- TEST_ADC ENDS

#endif // _ANALOG_MODULE
