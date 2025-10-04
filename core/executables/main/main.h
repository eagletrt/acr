#ifndef MAIN_H
#define MAIN_H

#include "acr.h"
#include <stdio.h>

void error_state(acr_error_t error);

void *led_runner();
void sig_handler(int signum);
void pin_setup(user_data_t *user_data);
void pin_interrupt(int gpio, int level, uint32_t tick, void *user_data);

#endif // MAIN_H
