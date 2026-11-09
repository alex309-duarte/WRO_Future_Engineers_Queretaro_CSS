// Raspberry Pi 5 GPIO access through libgpiod v2 for the Obstacle Challenge:
// Grove LED start button (button input, LED output).

#ifndef RASP_GPIO_H
#define RASP_GPIO_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <gpiod.h>

// BCM line numbers.
#define GPIO_BUTTON 4       // start button input (pull-up, pressed = low)
#define GPIO_LED_BUTTON 3   // LED inside the start button

// Opens the GPIO chip and requests the button and LED lines.
// Returns 0 on success, EXIT_FAILURE on error.
int Rasp_Gpio_Init(void);
// Releases the lines and the chip (safe to call more than once).
void Rasp_Gpio_Clean(void);
// Blocks until the start button is pressed or Ctrl+C, blinking the LED.
void Rasp_Gpio_Wait_For_Button(void);

#endif // RASP_GPIO_H
