//
// f p _ p o r t D i c t i o n a r y . h
//
// This file includes dictionary entries for the port functions
// The necessary includes to locate the functions must be
// referenced inside fport.h

// This line shows an example of one external definition
//   {"NAME","Description#(a)$",Function,value,flags},
//
// "NAME" is the word name
//
// "Description#(a)$" is the description ("#" marks start of second line)
//                                       ("$" is substituted by " -> "  )
//
// Function is the function that implements the word
// its prototype should be:
//      int32_t Function(ContextType *context,int32_t value)
//
// value will be set as the second parameter in the function calling
//
// flags can be:
//        DF_NI        Word cannot be executed in interactive mode
//
//        DF_NCOMPILE  Word cannot be compiled directly
//                     It won't probably be useful in port functions

// Time functions in timeModule.c/h
{"MS","Waits the indicated time in ms#(ms)$",timeFunction,TIME_F_SLEEP,0},

// GPIO Functions in gpioModule.c/h
// User LEDs commands
{"LedSet","Led u Set#(u)$",ledFunction,LED_F_SET,0},
{"LedClear","Led u Clear#(u)$",ledFunction,LED_F_CLEAR,0},
{"LedWrite","Led u Write flag#(f)(u)$",ledFunction,LED_F_WRITE,0},
{"LedRead","Led u Read flag#(n)$(f)",ledFunction,LED_F_READ,0},
{"LedBinSet","Led set binary pattern#(ub)$",ledBfunction,LED_F_BSET,0},
{"LedBinClear","Led clear binary pattern#(ub)$",ledBfunction,LED_F_BCLEAR,0},
{"LedBinWrite","Led binary write#(ub)$",ledBfunction,LED_F_BWRITE,0},
{"LedBinRead","Led binary read#$(ub)",ledBinaryRead,0,0},

// Digital I/O commands in gpioModule.c/h
{"DigitalOUtput","Digital u set to output #(u)$",gpioFunction,GPIO_F_OUTPUT,0},
{"DigitalOpenDrain","Digital u set to open drain #(u)$",gpioFunction,GPIO_F_OPEN_DRAIN,0},
{"DigitalInput","Digital u set to input #(u)$",gpioFunction,GPIO_F_INPUT,0},
{"DigitalPullUp","Digital u set to input with pull-up #(u)$",gpioFunction,GPIO_F_INPUT_UP,0},
{"DigitalPullDown","Digital u set to input with pull-down #(u)$",gpioFunction,GPIO_F_INPUT_DOWN,0},
{"DigitalRead","Digital u read as flag #(u)$(f)",gpioFunction,GPIO_F_READ,0},
{"DigitalSet","Digital u set#(u)$",gpioFunction,GPIO_F_SET,0},
{"DigitalClear","Digital u clear #(u)$",gpioFunction,GPIO_F_CLEAR,0},
{"DigitalWrite","Set digital u output value#(f)(u)$",gpioFunction,GPIO_F_WRITE,0},
{"DigitalReadOutput","Digital u read output#(u)$(f)",gpioFunction,GPIO_F_READOUT,0},
{"DigitalBinRead","Digital binary read#$(ub)",gpioBread,0,0},
{"DigitalBinSet","Digital binary set#(ub)$",gpioBfunction,GPIO_F_BSET,0},
{"DigitalBinClear","Digital binary clear#(ub)$",gpioBfunction,GPIO_F_BCLEAR,0},
{"DigitalBinWrite","Digital binary write#(ub)$",gpioBfunction,GPIO_F_BWRITE,0},
{"DigitalBinReadOutput","Digital binary read output#$(ub)",gpioBreadOut,0,0},

// Analog module in analog.c/h
// Analog commands
{"AnalogSingle","Analog channel u to single mode#(u)$",analogFunction,ANALOG_F_SINGLE,0},
{"AnalogDiff","Analog set differential u channel #(u)$",analogFunction,ANALOG_F_DIFFERENTIAL,0},
{"AnalogRead","Analog read u channel #(u)$(ua)",analogFunction,ANALOG_F_READ,0},
{"AnalogRead2","Analog read 2 channels #(n1)(n2)$(count1)(count2)",analogFunction,ANALOG_F_2READ,0},
{"AnalogMean","Analog u for mean (1..500000) #(u)$",analogFunction,ANALOG_F_NMEAN,0},
{"AnalogReadMean","Analog read channel and mean #(u)$(ua)",analogFunction,ANALOG_F_READ_MEAN,0},
{"AnalogReadMean2","Analog read 2 channels and mean #(u1)(u2)$(ua1)(ua2)",analogFunction,ANALOG_F_2READ_MEAN,0},
{"Single2mV","Analog convert single reading to mV Vdd #(ua)$(mV)",analogFunction,ANALOG_F_SINGLE_CONVERT,0},
{"Diff2mV","Analog convert differential reading to mV Vdd #(ua)$(mV)",analogFunction,ANALOG_F_DIFFERENTIAL_CONVERT,0},
{"mV2Single","Analog mV to single reading#(mV)$(ua)",analogFunction,ANALOG_F_MV2COUNTS,0},
{"AnalogWrite","Analog DAC write#(ua)$",analogFunction,ANALOG_F_DAC,0},
{"AnalogReadRef","Analog read Vref #$(ua)",analogFunction,ANALOG_F_READ_VREF,0},
{"AnalogReadTemp","Analog read Temperature#$(10*T)",analogFunction,ANALOG_F_READ_T,0},
{"AnalogCal","Analog calibrate Vdd #$(Vdd[mV])",analogFunction,ANALOG_F_CAL,0},
{"AnalogVdd","Analog give measured Vdd [updates calibration]#(Vdd[mV])$(Vref[mV])",analogFunction,ANALOG_F_USE_VDDMEAS,0},
{"AnalogVref","Analog give known Vref [updates calibration]#(Vref[mV])$(Vdd[mV])",analogFunction,ANALOG_F_USE_VREF,0},


// Gyroscope commands in buses.c/h
{"GyroRead","Gyroscope read 3D (8.75 mdps/count)#$(nz)(ny)(nx)",busesFunction,BUSES_GYR_READ,0},
{"GyroZero","Gyroscope set zero to current value",busesFunction,BUSES_GYR_ZERO,0},
{"GyroSetZero","Gyroscope set zero manually#(nx)(ny)(nz)$",busesFunction,BUSES_GYR_SET_ZERO,0},
{"GyroReadReg","Gyroscope read register#(ureg)$(uval)",internalRegistersFunction,INTREG_F_GRR,0},
{"GyroWriteReg","Gyroscope write register#(ureg)(uval)$",internalRegistersFunction,INTREG_F_GWR,0},

// Accelerometer commands in buses.c/h
{"ACcelRead","Accelerometer read 3D (61 ug/count)#$(nz)(ny)(nx)",busesFunction,BUSES_ACC_READ,0},
{"ACcelZero","Accelerometer X,Y set zero",busesFunction,BUSES_ACC_ZERO_XY,0},
{"ACcelSetZero","Accelerometer set zero manually#(nx)(ny)(nz)$",busesFunction,BUSES_ACC_SET_ZERO,0},
{"ACcelReadReg","Accelerometer read register#(ureg)$(uval)",internalRegistersFunction,INTREG_F_FRR,0},
{"ACcelWriteReg","Accelerometer write register#(ureg)(uval)$",internalRegistersFunction,INTREG_F_FWR,0},

// Magnetomenter commands in buses.c/h
{"MagRead","Magnetometer read 3D 670(X,Y)600(Z) count/gauss#$(z)(y)(x)",busesFunction,BUSES_MAG_READ,0},
{"MagSetZero","Magnetometer set zero manually#(nx)(ny)(nz)$",busesFunction,BUSES_MAG_SET_ZERO,0},
{"MagReadReg","Magnetometer read register#(ureg)$(uval)",internalRegistersFunction,INTREG_F_MRR,0},
{"MagWriteReg","Magnetometer write register#(ureg)(uval)$",internalRegistersFunction,INTREG_F_MWR,0},

// SPI commands in buses.c/h
{"SPIStart","SPI Start on slave u (0..3)#(u)$",busesFunction,SPI_F_START,0},
{"SPIEnd","SPI End transmission",busesFunction,SPI_F_END,0},
{"SPIByte","SPI exchange one Byte#(tx)$(rx)",busesFunction,SPI_F_EX8,0},
{"SPITransfer","SPI exchange n Bytes#(tx1)..(txn)(n)$(rx1)..(rxn)",spiNexangeFunction,0,0},
{"SPIFreq","SPI frequency#(kHz)$(kHz)",spiSetSpeed,0,0},
{"SPIMode","SPI mode 0..3#(mode)$",busesFunction,SPI_F_MODE,0},

// I2C commands in buses.c/h
{"I2CFreq","I2C frequency#(kHz)$(kHz)",i2cSetSpeed,0,0},
{"I2CReadReg","I2C Register Read#(addr)(nreg)$(value)",busesFunction,BUSES_I_RR,0},
{"I2CWriteReg","I2C Register Write#(addr)(nreg)(value)$",busesFunction,BUSES_I_WR,0},
{"ISCAN","I2C Address scan#$(add1)..(addn)(n)",busesFunction,BUSES_I_SCAN,0},
{"I2CTransfer","I2C Transfer#(addr)(d1w)..(duw)(uw)(ur) $ (dur)...(d1r)",i2cTransfer,0,0},

// Thread services
{"SIGNAL","Signal semaphore u#(u)$",semaphoreFunction,SEM_F_SIGNAL,0},
{"WAIT","Wait for semaphore u#(u)$",semaphoreFunction,SEM_F_WAIT,0},
{"SLIST","List status of semaphores",semaphoreFunction,SEM_F_LIST,0},
{"SRESET","Reset all semaphores to FREE",semaphoreFunction,SEM_F_RESET,0},
{"LOCK","Lock mutex u#(u)$",mutexFunction,MTX_F_LOCK,0},
{"UNLOCK","Unlock last locked mutex",mutexFunction,MTX_F_UNLOCK,0},
{"UNLOCKALL","Unlock all locked mutexes",mutexFunction,MTX_F_UNLOCK_ALL,0},

// Console status
{"CONSOLE?","Return console kind 0:None 1:Serial 2:USB#(u)$",consoleFunction,CONSOLE_F_ANY,0},

// Time functions in timeModule.c/h
{"TimerFreq","Set timer frequency of timer ut#(uf)(ut)$",timeFunction,TIME_F_TIMER_FREQ,0},
{"TimerWord","Set word callback of timer ut#(ut)$",timeFunction,TIME_F_TIMER_WORD,DF_DIRECTIVE},
{"TimerDelay","Polled delay during interval ui in timer ut#(ui)(ut)$",timeFunction,TIME_F_DELAY,0},
{"TimerRepeat","Set timer ut in repeat mode with ui interval#(ui)(ut)$",timeFunction,TIME_F_REPEAT,0},
{"TimerPause","Pause timer ut#(ut)$",timeFunction,TIME_F_PAUSE,0},
{"TimerOneShot","Start timer ut in one shot mode interval ui#(ui)(ut)$",timeFunction,TIME_F_ONE,0},
{"TimerRESET","Pauses all timers and removes callback words$",timeFunction,TIME_F_RESET,0},

// PWM Module
{"PWMSet","Set PWM Channel uch to ui interval#(ui)(uch)$",pwmFunction,PWM_F_CHON,0},
{"PWMReset","Reset PWM Channel uch#(uch)$",pwmFunction,PWM_F_CHOFF,0},
{"PWMFreq","Set PWM clock frequency#(uf)$",pwmFunction,PWM_F_FREQ,0},
{"PWMPeriod","Set PWM period in clock cycles#(up)$",pwmFunction,PWM_F_PERIOD,0},
{"PWMSTOP","Stop all PWM operations",pwmFunction,PWM_F_STOP,0},


