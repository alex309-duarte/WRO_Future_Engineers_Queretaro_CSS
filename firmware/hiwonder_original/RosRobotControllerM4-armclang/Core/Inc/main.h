/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BATTERY_Pin GPIO_PIN_0
#define BATTERY_GPIO_Port GPIOB
#define SERIAL_SERVO_TX_EN_Pin GPIO_PIN_7
#define SERIAL_SERVO_TX_EN_GPIO_Port GPIOE
#define SERIAL_SERVO_RX_EN_Pin GPIO_PIN_8
#define SERIAL_SERVO_RX_EN_GPIO_Port GPIOE
#define TIM1_CH1_Pin GPIO_PIN_9
#define TIM1_CH1_GPIO_Port GPIOE
#define LED_SYS_Pin GPIO_PIN_10
#define LED_SYS_GPIO_Port GPIOE
#define TIM1_CH2_Pin GPIO_PIN_11
#define TIM1_CH2_GPIO_Port GPIOE
#define TIM1_CH3_Pin GPIO_PIN_13
#define TIM1_CH3_GPIO_Port GPIOE
#define TIM1_CH4_Pin GPIO_PIN_14
#define TIM1_CH4_GPIO_Port GPIOE
#define IMU_ITR_Pin GPIO_PIN_12
#define IMU_ITR_GPIO_Port GPIOB
#define IMU_ITR_EXTI_IRQn EXTI15_10_IRQn
#define MASTER_TX_Pin GPIO_PIN_8
#define MASTER_TX_GPIO_Port GPIOD
#define MASTER_RX_Pin GPIO_PIN_9
#define MASTER_RX_GPIO_Port GPIOD
#define LCD_BLK_Pin GPIO_PIN_11
#define LCD_BLK_GPIO_Port GPIOD
#define LCD_CS_Pin GPIO_PIN_12
#define LCD_CS_GPIO_Port GPIOD
#define LCD_DC_Pin GPIO_PIN_13
#define LCD_DC_GPIO_Port GPIOD
#define LCD_RES_Pin GPIO_PIN_14
#define LCD_RES_GPIO_Port GPIOD
#define SERIAL_SERVO_TX_Pin GPIO_PIN_6
#define SERIAL_SERVO_TX_GPIO_Port GPIOC
#define SERIAL_SERVO_RX_Pin GPIO_PIN_7
#define SERIAL_SERVO_RX_GPIO_Port GPIOC
#define PWM_SERVO_3_Pin GPIO_PIN_8
#define PWM_SERVO_3_GPIO_Port GPIOC
#define PWM_SERVO_4_Pin GPIO_PIN_9
#define PWM_SERVO_4_GPIO_Port GPIOC
#define BUZZER_Pin GPIO_PIN_8
#define BUZZER_GPIO_Port GPIOA
#define DBG_TX_Pin GPIO_PIN_9
#define DBG_TX_GPIO_Port GPIOA
#define DBG_RX_Pin GPIO_PIN_10
#define DBG_RX_GPIO_Port GPIOA
#define PWM_SERVO_1_Pin GPIO_PIN_11
#define PWM_SERVO_1_GPIO_Port GPIOA
#define PWM_SERVO_2_Pin GPIO_PIN_12
#define PWM_SERVO_2_GPIO_Port GPIOA
#define SBUS_RX_Pin GPIO_PIN_2
#define SBUS_RX_GPIO_Port GPIOD
#define MOTOR_ENABLE_Pin GPIO_PIN_3
#define MOTOR_ENABLE_GPIO_Port GPIOD
#define BLE_TX_Pin GPIO_PIN_5
#define BLE_TX_GPIO_Port GPIOD
#define BLE_RX_Pin GPIO_PIN_6
#define BLE_RX_GPIO_Port GPIOD
#define KEY2_Pin GPIO_PIN_0
#define KEY2_GPIO_Port GPIOE
#define KEY1_Pin GPIO_PIN_1
#define KEY1_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
