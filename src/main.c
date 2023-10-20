#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <pigpio.h>
#include <unistd.h>

#include "defines.h"
#include "led.h"

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

    led_t *led_gn = led_new(P_LED_GN);
    led_t *led_rd = led_new(P_LED_RD);

    led_set_state(led_gn, 100, 100);
    led_set_state(led_rd, 1000, 500);

    while(1) {
	    led_run();
	    usleep(1e2);
	    /*printf("%d(yl) %d(or) %d(bl) %d(mode)\n",
			    gpioRead(P_BTN_YL),
			    gpioRead(P_BTN_OR),
			    gpioRead(P_BTN_BL),
			    gpioRead(P_BTN_MODE)
		  );*/

    }

    return EXIT_SUCCESS;
}
