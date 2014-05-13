/*
 * Base.h
 * Some basic definitions for STM32F3
 *
 * 14/05/2013 First version
*/

/*
 * Some documentation:
 *
 * ChibiOS/RT Documentation
 * http://chibios.sourceforge.net/html/index.html
 */

#ifndef _BASE_INCLUDE
#define _BASE_INCLUDE

// Bit macro

#define BIT(n) (1<<(n))

// 32 direct bit macros

#define BIT0   0x01
#define BIT1   0x02
#define BIT2   0x04
#define BIT3   0x08
#define BIT4   0x10
#define BIT5   0x20
#define BIT6   0x40
#define BIT7   0x80
#define BIT8   0x100
#define BIT9   0x200
#define BIT10  0x400
#define BIT11  0x800
#define BIT12  0x1000
#define BIT13  0x2000
#define BIT14  0x4000
#define BIT15  0x8000
#define BIT16  0x10000
#define BIT17  0x20000
#define BIT18  0x40000
#define BIT19  0x80000
#define BIT20  0x100000
#define BIT21  0x200000
#define BIT22  0x400000
#define BIT23  0x800000
#define BIT24  0x1000000
#define BIT25  0x2000000
#define BIT26  0x4000000
#define BIT27  0x8000000
#define BIT28  0x10000000
#define BIT29  0x20000000
#define BIT30  0x40000000
#define BIT31  0x80000000

// ChibiOS Disable / Enable

#define DISABLE  chSysLock()
#define ENABLE   chSysUnlock()

// SLEEP
#define SLEEP_MS(x) chThdSleep(x)

#endif // Base.h


