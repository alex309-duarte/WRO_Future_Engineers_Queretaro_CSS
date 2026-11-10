/**
 * @file app.c
 * @author Lu Yongping (Lucas@hiwonder.com)
 * @brief 主应用逻辑
 * @version 0.1
 * @date 2023-05-08
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "cmsis_os2.h"
#include "led.h"
#include "lwmem_porting.h"
#include "global.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "global.h"
#include "adc.h"
#include "u8g2_porting.h"
#include "packet_reports.h"
#include "packet_handle.h"
#include "serial_servo.h"


void buzzers_init(void);
void buttons_init(void);
void leds_init(void);
void motors_init(void);
void serial_servo_init(void);
void pwm_servos_init(void);
void sbus_init(void);
void chassis_init(void);


void button_event_callback(ButtonObjectTypeDef *button,  ButtonEventIDEnum event)
{
    PacketReportKeyEventTypeDef report = {
        .key_id = button->id,
        .event = (uint8_t)(int)event,
    };
    packet_transmit(&packet_controller, PACKET_FUNC_KEY, &report, sizeof(PacketReportKeyEventTypeDef));
	if(event == BUTTON_EVENT_CLICK) {
		buzzer_didi(buzzers[0], 2000, 50, 50, 1);
	}
}

void app_task_entry(void *argument)
{
    extern osTimerId_t led_timerHandle;
    extern osTimerId_t buzzer_timerHandle;
    extern osTimerId_t button_timerHandle;
    extern osTimerId_t battery_check_timerHandle;
    extern osMessageQueueId_t moving_ctrl_queueHandle;
    extern osMessageQueueId_t bluetooth_tx_queueHandle;

    motors_init();
    pwm_servos_init();
	serial_servo_init();
    leds_init();
    buzzers_init();
    buttons_init();
	
    button_register_callback(buttons[0], button_event_callback);
    button_register_callback(buttons[1], button_event_callback);

    osTimerStart(led_timerHandle, LED_TASK_PERIOD);
    osTimerStart(buzzer_timerHandle, BUZZER_TASK_PERIOD);
    osTimerStart(button_timerHandle, BUTTON_TASK_PERIOD);
    osTimerStart(battery_check_timerHandle, BATTERY_TASK_PERIOD);
    packet_handle_init();

//    char msg = '\0';
//    uint8_t msg_prio;
//    osMessageQueueReset(moving_ctrl_queueHandle);

    chassis_init();
    set_chassis_type(CHASSIS_TYPE_JETACKER);

    for(;;) {
		osDelay(10000);
    }
}



