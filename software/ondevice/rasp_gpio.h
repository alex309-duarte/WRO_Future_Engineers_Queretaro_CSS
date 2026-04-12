#ifndef RASP_GPIO_H
#define RASP_GPIO_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <gpiod.h>

#define GPIO_BUTTON 4
#define GPIO_LED_BUTTON 17
#define GPIO_RELE 12

int Rasp_Gpio_Init(void);
void Rasp_Gpio_Clean(void);
void Rasp_Gpio_Wait_For_Button(void);
void Rasp_Gpio_Power_On_Spike(void);

#endif // RASP_GPIO_H