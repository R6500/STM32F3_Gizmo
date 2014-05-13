/*
 gpioModule.h
 GPIO and LED functions header file

 PADs are programmed by default as input without pullup/pulldown
 */

#ifndef _GPIO_MODULE
#define _GPIO_MODULE

// LED Mask
#define LED_MASK (BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14|BIT15)

// ledOn macro
#define ledOn(n) (LEDS_PORT->BSRR.H.set)=LedArray[n]

// ledOff macro
#define ledOff(n) (LEDS_PORT->BSRR.H.clear)=LedArray[n]

// ledRead macro (Reads led ODR register)
#define ledRead(n) ((LEDS_PORT->ODR)&(LedArray[n]))

// Arguments of ledfunction
#define LED_F_SET      1
#define LED_F_CLEAR    2
#define LED_F_WRITE    3
#define LED_F_READ     4

// Arguments of ledBfunction
#define LED_F_BSET      1
#define LED_F_BCLEAR    2
#define LED_F_BWRITE    3

// Arguments of gpioFunction
#define GPIO_F_OUTPUT     1
#define GPIO_F_OPEN_DRAIN 2
#define GPIO_F_INPUT      3
#define GPIO_F_INPUT_UP   4
#define GPIO_F_INPUT_DOWN 5

#define GPIO_F_READ       6
#define GPIO_F_SET        7
#define GPIO_F_CLEAR      8
#define GPIO_F_WRITE      9
#define GPIO_F_READOUT   10

// Arguments of gpioBfunction
#define GPIO_F_BSET      1
#define GPIO_F_BCLEAR    2
#define GPIO_F_BWRITE    3

// GPIO Mask
#define GPIO_MASK (BIT(GPIO0_PIN)|BIT(GPIO1_PIN)|BIT(GPIO2_PIN)|\
		           BIT(GPIO3_PIN)|BIT(GPIO4_PIN)|BIT(GPIO5_PIN)|\
		           BIT(GPIO6_PIN)|BIT(GPIO7_PIN)|BIT(GPIO8_PIN)|\
		           BIT(GPIO9_PIN))

// Gpio ON macro
#define GpioOn(n) (GPIO_PORT->BSRR.H.set)=BIT(GpioArray[n])

// Gpio OFF macro
#define GpioOff(n) (GPIO_PORT->BSRR.H.clear)=BIT(GpioArray[n])

// Function prototypes
void gpioModuleInit(void);

// Command function prototypes
int32_t ledFunction(ContextType *context,int32_t value);
int32_t ledBfunction(ContextType *context,int32_t value);
int32_t ledBinaryRead(ContextType *context,int32_t value);
int32_t gpioFunction(ContextType *context,int32_t value);
int32_t gpioBread(ContextType *context,int32_t value);
int32_t gpioBreadOut(ContextType *context,int32_t value);
int32_t gpioBfunction(ContextType *context,int32_t value);
int32_t gpioButtonRead(ContextType *context,int32_t value);

#endif // _GPIO_MODULE

