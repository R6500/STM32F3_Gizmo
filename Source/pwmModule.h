/*
 pwmModule.h
 Pwm functions header file
 */

#ifndef _PWM_MODULE
#define _PWM_MODULE

// Number of lines
#define PWM_NLINES	3

// Hardware used for this module
#define PWM_PORT	GPIOB
#define PWM_PIN1	4	// TIM3 CH1
#define PWM_PIN2	5   // TIM3 CH2
#define PWM_PIN3	0   // TIM3 CH3

// Driver used
#define PWM_DRIVER   PWMD3

// Default PWM values
#define PWM_DEF_FREQ	   10000   // 10 kHz
#define PWM_DEF_PERIOD      1000   // 100ms (1000 clock cycles)

// Max interval
#define PWM_MAX_INTERVAL   65535   // 16 bit

// Values of pwmStatus variable
#define PWMS_OFF	0 // Not used
#define PWMS_ON	    1 // Active

// Function prototypes
void pwmModuleInit(void);

// Command function prototypes
int32_t pwmFunction(ContextType *context,int32_t value);
#define PWM_F_CHON	   0   // Activate one channel
#define PWM_F_CHOFF    1   // Deactivate one channel
#define PWM_F_FREQ     3   // Set frequency
#define PWM_F_PERIOD   4   // Set period
#define PWM_F_STOP     5   // Stop the driver

#endif  //_PWM_MODULE


