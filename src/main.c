#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <pigpio.h>
#include <unistd.h>

#include "defines.h"

int main(void) {

    printf("ACR: Advanced Cone Registration\n");

    gpioInitialise();

    gpioSetMode(P_LED_GN, PI_OUTPUT);
    gpioSetMode(P_LED_RD, PI_OUTPUT);
    gpioSetMode(P_BTN_YL, PI_INPUT);
    gpioSetMode(P_BTN_OR, PI_INPUT);
    gpioSetMode(P_BTN_BL, PI_INPUT);
    gpioSetMode(P_BTN_MODE, PI_INPUT);

    gpioSetPullUpDown(P_BTN_YL, PI_PUD_UP);
    gpioSetPullUpDown(P_BTN_OR, PI_PUD_UP);
    gpioSetPullUpDown(P_BTN_BL, PI_PUD_UP);
    gpioSetPullUpDown(P_BTN_MODE, PI_PUD_UP);

    while(1) {
	    printf("%d(yl) %d(or) %d(bl) %d(mode)\n",
			    gpioRead(P_BTN_YL),
			    gpioRead(P_BTN_OR),
			    gpioRead(P_BTN_BL),
			    gpioRead(P_BTN_MODE)
		  );
	   gpioWrite(P_LED_GN, 1 - gpioRead(P_BTN_YL));
	   usleep(1e5);
    }

    return EXIT_SUCCESS;
}
