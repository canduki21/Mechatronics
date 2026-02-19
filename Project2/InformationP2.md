Information About Hardware & Software for Project 2
======

* I'm using an Arduino Mega atmega2560
  
* 2.8" TFT Touch Shield for Arduino with Resistive Touch Screen v2 with the TSC2007 chip

* Libraries needed:
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Wire.h>
#include <Adafruit_TSC2007.h>
#include <Servo.h>


* I'm using 220 omhs reisitors in each of the LED pins connectors and a 10k for the press button

* Pin Layout
![alt text](https://github.com/canduki21/Mechatronics/blob/main/Project-1/PINALYOUT-P1.png)
* MX Pin Layout
![alt text](https://github.com/canduki21/Mechatronics/blob/main/Project-1/MXLAYOUT.png)
* Clock Settings
 ```
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 7199;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 99;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  HAL_TIM_Base_Init(&htim2);
```
The clock configuration, TIM2 is running from a 72 MHz timer clock. Using a prescaler of 7199 divides this clock down to 10 kHz, meaning the timer counter increments every 0.1 ms. The period value of 99 then makes the counter run for 100 counts before generating an update event. As a result, the timer overflows every 10 ms, which is exactly 0.01 seconds, or 1/100 of a second.

