#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

#ifndef ACR_NO_PIGPIO
#include <pigpio.h>
#else

#define PI_INPUT 1
#define PI_OUTPUT 0
#define PI_PUD_UP 0

#define PI_INIT_FAILED -1

typedef void (*eventFuncEx_t)(int event, int level, uint32_t tick, void *userdata);

int gpioInitialise();
int gpioTerminate();

int gpioRead(int);
int gpioWrite(int, int);
int gpioSetMode(int, int);
int gpioSetPullUpDown(int, int);
void gpioSetAlertFuncEx(int pin, eventFuncEx_t func, void *user_data);

#endif // ACR_NO_PIGPIO

int gpioSkipForDebounce(int pin, int state);

#endif // GPIO_H