#ifndef LED_H
#define LED_H

#include "defines.h"

#ifndef LED_MAX_LEDS
#define LED_MAX_LEDS 4
#endif//LED_MAX_LEDS

#define LED_DEFAULT 0, 100
#define LED_ERROR_BLINK 100, 100

typedef struct _led_t led_t;

led_t *led_new(int pin);

// Called every while loop
void led_run();

// Change blink state
void led_set_state(led_t *led, int on_ms, int off_ms);

void led_blink_once(led_t *led, int on_ms);

void led_on(led_t *led);
void led_off(led_t *led);

#endif // LED_H
