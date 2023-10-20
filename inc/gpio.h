#ifndef GPIO_H
#define GPIO_H

#ifndef ACR_NO_PIGPIO
#include <pigpio.h>
#else

#define PI_INPUT 1
#define PI_OUTPUT 0
#define PI_PUD_UP 0

#define PI_INIT_FAILED -1

int gpioInitialise();
int gpioTerminate();

int gpioRead(int);
int gpioWrite(int, int);
int gpioSetMode(int, int);
int gpioSetPullUpDown(int, int);


#endif // ACR_NO_PIGPIO

#endif // GPIO_H