#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <pigpio.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>

#include "defines.h"
#include "led.h"
#include "gpslib/gps_interface.h"

void sig_handler(int signum) {
	if(signum == SIGKILL || signum == SIGINT) {
		gpioWrite(P_LED_GN, 0);
		gpioWrite(P_LED_RD, 0);
		gpioTerminate();
		exit(EXIT_FAILURE);
	}
}

int main(void) {
    printf("ACR: Advanced Cone Registration\n");

    if(gpioInitialise() == PI_INIT_FAILED) {
	    printf("Error initializing pigpio\n");
	    exit(EXIT_FAILURE);
    }

	signal(SIGINT, sig_handler);
	signal(SIGKILL, sig_handler);

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

    gps_serial_port gps;
    // int res = gps_interface_open(&gps, "/dev/ttyACM0", B230400);
    int res = gps_interface_open_file(&gps, "/home/philpi/gps_0.log");
    if(res == -1) {
	    printf("Could not open gps serial port\n");
	    exit(EXIT_FAILURE);
    }

    const char start_sequence[GPS_MAX_START_SEQUENCE_SIZE];
    char line[GPS_MAX_LINE_SIZE];

    while(1) {
	    int start_size, line_size;
	    gps_interface_get_line(&gps, start_sequence, &start_size, line, &line_size, true);
	    printf("%s\n", line);
	    led_run();

    }

    return EXIT_SUCCESS;
}
