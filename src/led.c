#include "led.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <sys/time.h>
#include <pigpio.h>

typedef struct _led_t{
	int PIN;
	int on_us;
	int off_us;
	int state;
	uint32_t t;
}led_t;

static int last_led = -1;
led_t leds[LED_MAX_LEDS];

static uint32_t get_t() {
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC_RAW, &t);
	uint32_t us = (t.tv_sec * 1e6) + (t.tv_nsec * 1e-3);
	return us;
}

led_t *led_new(int pin) {
	assert(last_led + 1 < LED_MAX_LEDS);
	last_led ++;

	leds[last_led].PIN = pin;
	leds[last_led].on_us = 0;
	leds[last_led].off_us = 0;
	leds[last_led].state = 0;
	leds[last_led].t = get_t();

	return &(leds[last_led]);
}

void led_set_state(led_t *led, int on_ms, int off_ms) {
	assert(led);
	led->on_us = on_ms * 1e3;
	led->off_us = off_ms * 1e3;
	led->t = get_t();

	gpioWrite(led->PIN, 0);
}

void led_run() {
	uint32_t t = get_t();
	for(int i = 0; i <= last_led; ++i) {
		int cond = (t - leds[i].t) <= leds[i].on_us;
		gpioWrite(leds[i].PIN, cond);
		if(t - leds[i].t > (leds[i].on_us + leds[i].off_us)) {
			leds[i].t = get_t();
		}
	}
}
