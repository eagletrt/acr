#include "utils.h"

#include <time.h>

uint64_t get_t() {
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC_RAW, &t);
	uint64_t us = (t.tv_sec * 1e6) + (t.tv_nsec * 1e-3);
	return us;
}