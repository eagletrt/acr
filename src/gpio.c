#include "gpio.h"
#include "defines.h"
#include "utils.h"

#ifdef ACR_NO_PIGPIO
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>


int setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1) {
        perror("fcntl");
        return -1;
    }

    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1) {
        perror("fcntl");
        return -1;
    }

    return 0;
}

char pinToKey(int pin) {
    switch(pin) {
        case P_BTN_YL:
            return 'y';
        case P_BTN_BL:
            return 'b';
        case P_BTN_OR:
            return 'o';
        case P_BTN_MODE:
            return 'm';
    }
    return ' ';
}

static char read_key() {
  char key;
  struct termios old_tio, new_tio;
  // save current terminal settings
  tcgetattr(STDIN_FILENO, &old_tio);
  // copy setting into new structure
  new_tio = old_tio;
  // disable caching
  cfmakeraw(&new_tio);
  // set new settings
  tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
  // wait for keypress
  key = getchar();
  // reset initial temrinal setings
  tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
  return key;
}

int gpioInitialise(){
    return 0;
}
int gpioTerminate(){
    return 0;
}
int gpioRead(int pin){
    setNonBlocking(fileno(stdin));
    char c = read_key();
    if(c == 'q') {
        printf("Exiting\n");
        exit(EXIT_SUCCESS);
    }
    return c == pinToKey(pin);
}
int gpioWrite(int pin, int state){
    // printf("Pin %d set to %d\n", pin, state);
    return 0;
}
int gpioSetMode(int, int){
    return 0;
}
int gpioSetPullUpDown(int, int){
    return 0;
}

#endif // ACR_NO_PIGPIO

typedef struct deb_t {
    int state;
    uint64_t t;
}deb_t;
static deb_t debounce_pins[MAX_PINS];

int gpioSkipForDebounce(int pin, int state) {
    int ret = 0;
    debounce_pins[pin].state = state;

    if(get_t() - debounce_pins[pin].t < DEBOUNCE_US) {
        ret = 1;
    }
    debounce_pins[pin].t = get_t();
    return ret;
}