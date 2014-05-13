/*
 thservices.h
 Thread services header file
 */

#ifndef _THSERVICES
#define _THSERVICES

// Function prototypes
void thservicesInit(void);

// Command functions
int32_t semaphoreFunction(ContextType *context,int32_t value);
#define SEM_F_SIGNAL   0
#define SEM_F_WAIT     1
#define SEM_F_LIST     2
#define SEM_F_RESET    3

int32_t mutexFunction(ContextType *context,int32_t value);
#define MTX_F_LOCK         0
#define MTX_F_UNLOCK       1
#define MTX_F_UNLOCK_ALL   2


#endif // _THSERVICES

