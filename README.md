# ACR
Advanced Cone Registration

Tool to collect data fron track-walks before races.
With a raspberry and a GPS connected to it, ACR allows to log cones positions.

## Shield
The shield uses:
- 4 buttons:
    - Yellow (YL)(PIN 22): when pressed registers a yellow cone.
    - Blue   (BL)(PIN 23): registers a blue cone.
    - Orange (OR)(PIN 24): registers an orange cone.
    - Mode (Mode)(PIN 27): toggles a continuous logging (trajectory mode).
- 2 leds:
    - Green (GN)(PIN 5): cone registration
    - Red   (RD)(PIN 6): trajectory logging or error type
These pins are defined in **defines.h**.

## Cones logging
Pressing Yellow, Blue and Orange buttons will log the corresponding cone.

The position saved can be the last received from the GPS or an average of the next positions. This behaviour is controlled by **CONE_ENABLE_MEAN** in **defines.h**.  
The mean is made in the short form:
$mean = w * mean + (1 - w) * sample$.  
w is a weight from 0 to 1 and defines the window of the average. Setting w to 0 removes the mean, increasing the value increases the window. A value of 0.9 corresponds to a window of 10 elements.
w value is **CONE_MEAN_COMPLEMENTARY** in **defines.h**.

A cone can be sampled at least **CONE_REPRESS_US** microseconds later than the previous cone.

The cone registration is confirmed by the Green led. When one of the three buttons is pressed, the green led turns on, when the cone is saved it turns off.

### File format
Cone positions are saved in CSV format. The file is in his folder called cone_\<number>, where number increases for each session.
~~~csv
timestamp,cone_id,cone_name,lat,lon,alt
000000001,0,YELLOW,0.0,0.0,0.0
000000002,1,BLUE,0.0,0.0,0.0
000000003,2,ORANGE,0.0,0.0,0.0
~~~
timestamp is in microseconds.

Cone id are:
- 0: Yellow
- 1: Blue
- 2: Orange

The session is created only if one of the cone buttons is pressed, in that case the folder path is printed on the terminal.

## Continuous logging
Enables logging of all the data received from the GPS, saves them in CSV format.

To enable continuous logging use Mode button, it acts as a toggle.  
The status of this logging is shown via the Red led: ON when logging, OFF otherwise.

### File format
Files are in folder called trajectory_\<number>, where number is increasing for each session.

## Errors
Leds are used to notify about errors.
Errors are defined in **main.h** in **error_t**.
~~~c
typedef enum error_t {
    ERROR_GPIO_INIT,
    ERROR_GPS_NOT_FOUND,
    ERROR_GPS_READ,
    ERROR_CONE_SESSION_SETUP,
    ERROR_CONE_SESSION_START,
    ERROR_FULL_SESSION_SETUP,
    ERROR_FULL_SESSION_START,

    ERORR_SIZE
} error_t;
~~~

Based on the error a different blink pattern is showed on the Red led.  
Basically the red led "counts" the error, so if the error is ERROR_GPS_NOT_FOUND, then the led blinks twice. The pattern repeats every two seconds.

### Issue
The error GPIO_INIT cannot be displayed, the gpio are not initialized and so the led cannot be turned on.

### Reset
When in error state the executable can be killed pressing Blue and Orange buttons simultaneously.  
This action will self-kill the process.  
Assuming the process is managed by systemctl, then it will be restarted automatically.