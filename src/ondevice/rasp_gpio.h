// Raspberry Pi 5 GPIO access through libgpiod v2 for the Open Challenge:
// Grove LED start button (button input, LED output) and the relay output that
// powered the LEGO SPIKE hub in design v6.

#ifndef RASP_GPIO_H
#define RASP_GPIO_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <gpiod.h>

// BCM line numbers.
#define GPIO_BUTTON 4       // start button input (pull-up, pressed = low)
#define GPIO_LED_BUTTON 3   // LED inside the start button
#define GPIO_RELE 2         // relay that switched on the SPIKE hub (design v6)

// Opens the GPIO chip and requests the button and output lines.
// Returns 0 on success, EXIT_FAILURE if the chip cannot be opened.
int Rasp_Gpio_Init(void);
// Releases the lines and the chip.
void Rasp_Gpio_Clean(void);
// Blocks until the start button is pressed, blinking its LED meanwhile.
void Rasp_Gpio_Wait_For_Button(void);
// Pulses the relay to power on the SPIKE hub (design v6 only).
void Rasp_Gpio_Power_On_Spike(void);

#endif // RASP_GPIO_H