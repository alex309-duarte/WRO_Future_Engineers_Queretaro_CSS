#include "adc.h"
#include "global.h"
#include "global_conf.h"
#include "buzzer.h"
#include "packet_reports.h"
#include "packet.h"

float battery_volt = 0.0f; /* 电池电压全局变量, 单位 v */
static uint16_t adc_value[2];

extern osMessageQueueId_t lvgl_event_queueHandle;
#if ENABLE_BLUETOOTH&&ENABLE_BLUETOOTH_BATTERY_REPORT /* 开启蓝牙电压报告后引入 */
extern osMessageQueueId_t bluetooth_tx_queueHandle; /* 蓝牙数据发送队列 */
#endif


void battery_check_timer_callback(void *argument)
{
    if(adc_value[0] != 0 && adc_value[0] != 4095) { /* 内部参考电压不能为0, 否则无法计算 */
        //float vdda = 3300.0f * ((float)(*((__IO uint16_t*)(0x1FFF7A2A)))) / ((float)adc_value[0]);
        //float volt = vdda / 4095.0f * ((float)adc_value[1]) * 11.0f ; /* 100k + 10k 电阻分压， 实际电压是测量电压的11倍 */
		float volt = 1210.0f / ((float)adc_value[0]) * ((float)adc_value[1]) * 11.0f;
        volt = volt > 20000 ? 0 : volt; /* ADC读取值超过最大允许供电电压，数据错误 */
        battery_volt = battery_volt == 0 ? volt : battery_volt * 0.95f + volt * 0.05f;
    }
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_value, 2);
    static int battery_report_count = 0;
    battery_report_count++;

    if(battery_report_count > (int)(1 * 1000 / BATTERY_TASK_PERIOD)) { /* 定时发送蓝牙电压报告 */
        battery_report_count = 0;
		PacketReportBatteryVoltageTypeDef report;
		report.sub_cmd = 0x04;
		report.voltage = (int)(battery_volt + 0.5f);
        packet_transmit(&packet_controller, PACKET_FUNC_SYS, &report, sizeof(PacketReportBatteryVoltageTypeDef));
		
#if ENABLE_LVGL
        ObjectTypeDef object;
        object.structure.type_id = OBJECT_TYPE_ID_BATTERY_VOLTAGE;
        *((uint16_t*)object.structure.data) = (int)(battery_volt + 0.5f);
        osMessageQueuePut(lvgl_event_queueHandle, &object, 0, 0);
#endif

#if  ENABLE_BLUETOOTH&&ENABLE_BLUETOOTH_BATTERY_REPORT
        char msg[8];
        sprintf(&msg[1], "V%dV", (int)(battery_volt + 0.5f)); /* 组织蓝牙电量消息 */
        msg[0] = strlen(&msg[1]);
        osMessageQueuePut(bluetooth_tx_queueHandle, msg, 0, 0); /* 压入发送队列 */
#endif
    }

#if ENABLE_BATTERY_LOW_ALARM
    static int count = 0;
    if(battery_volt < BATTERY_LOW_ALARM_THRESHOLD && battery_volt > 4900) {
        count++;
    } else {
        count = 0;
    }
    if(count > (int)(10 * 1000 / BATTERY_TASK_PERIOD)) { /* 每 10s 触发一次警报声 */
        buzzer_didi(buzzers[0], 2100, 800, 200, 5);
        count = 0;
    }
#endif
}
